#include "QMainController.h"
#include "QDebugConsoleServer.h"
#include <QDateTime>
#include <QMessageLogContext>
#include <QMetaObject>
#include <QRunnable>
#include <QThreadPool>
#include <QDebug>
#include <string_view>
#include <utility>
#include "../ref/iris/src/iris_lua.h"

std::atomic<QMainController*> QMainController::gLogHandlerTarget = nullptr;
QtMessageHandler QMainController::gPreviousQtMessageHandler = nullptr;

QMainController::QMainController(QObject* parent) : QObject(parent), mainWarp(asyncWorker) {
	Warp::preempt_guard_t guard(mainWarp, 0);
	guard.cleanup();

	asyncWorker.set_priority_task_handler([this](AsyncWorker::task_base_t* task, size_t& priority) {
		if (static_cast<ptrdiff_t>(priority) < 0) {
			mainWarp.queue_routine([this, task]() {
				asyncWorker.execute_task(task);
			});

			flushMain();
		} else {
			QThreadPool::globalInstance()->start([this, task]() {
				asyncWorker.execute_task(task);
			}, 0);
		}

		return true;
	}, 1);

	asyncWorker.start();
	installQtMessageHandler();
	initializeLuaState();

	debugConsoleServer = std::make_unique<QDebugConsoleServer>(this, this);
	if (!debugConsoleServer->startListening(8068)) {
		qWarning() << "Failed to start debug web console on port 8068.";
	} else {
		appendApplicationLog(QStringLiteral("Debug web console is listening on http://127.0.0.1:8068/"), QStringLiteral("system"));
	}
}

void QMainController::flushMain() {
	if (queueState.load(std::memory_order_acquire) != Warp::queue_state_t::pending) {
		if (queueState.exchange(Warp::queue_state_t::pending, std::memory_order_acq_rel) == Warp::queue_state_t::idle) {
			QMetaObject::invokeMethod(this, [this]() {
				do {
					queueState.store(Warp::queue_state_t::executing, std::memory_order_release);
					Warp::poll({ mainWarp });
				} while (queueState.exchange(Warp::queue_state_t::idle, std::memory_order_acquire) == Warp::queue_state_t::pending);
			}, Qt::QueuedConnection);
		}
	}
}

QMainController::~QMainController() {
	debugConsoleServer.reset();
	shutdownLuaState();
	uninstallQtMessageHandler();

	if (!asyncWorker.is_terminated()) {
		asyncWorker.terminate();
	}

	asyncWorker.join();
	mainWarp.yield();

	while (Warp::poll({ mainWarp })) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

void QMainController::appendApplicationLog(const QString& message, const QString& category) {
	if (QThread::currentThread() != thread()) {
		invokeMain([this, message, category]() {
			appendApplicationLog(message, category);
		});
		return;
	}

	appendConsoleLogLocked(category, message);
}

void QMainController::clearConsoleOutput() {
	logEntries.clear();
	++consoleStateVersion;
}

QList<DebugConsoleLogEntry> QMainController::getConsoleLogSnapshot() const {
	return logEntries;
}

QStringList QMainController::getExecutionHistorySnapshot() const {
	return executionHistory;
}

quint64 QMainController::getConsoleStateVersion() const {
	return consoleStateVersion;
}

void QMainController::initializeLuaState() {
	luaState = luaL_newstate();
	if (luaState == nullptr) {
		appendApplicationLog(QStringLiteral("Failed to initialize Lua state."), QStringLiteral("error"));
		return;
	}

	luaL_openlibs(luaState);
	iris::iris_lua_t(luaState).set_global("print", [this](iris::iris_lua_t lua, iris::iris_lua_t::stackindex_t stackIndex) {
		luaPrint(lua, stackIndex);
	});
}

void QMainController::shutdownLuaState() {
	if (luaState != nullptr) {
		lua_close(luaState);
		luaState = nullptr;
	}
}

void QMainController::luaPrint(iris::iris_lua_t lua, iris::iris_lua_t::stackindex_t stackIndex) {
	QStringList fragments;
	int i = stackIndex.index;
	int top = lua.native_get_top();
	for (int i = stackIndex.index; i <= top; i++) {
		size_t len;
		const char* str = lua_tolstring(lua.get_state(), i, &len);
		fragments.append(str == nullptr ? "" : QString::fromUtf8(str, len));
	}

	appendApplicationLog(fragments.join(QStringLiteral("\t")), QStringLiteral("lua"));
}

void QMainController::appendConsoleLogLocked(const QString& category, const QString& message) {
	DebugConsoleLogEntry entry;
	entry.id = nextLogEntryId++;
	entry.timestamp = createLogTimestamp();
	entry.category = category;
	entry.message = message;

	logEntries.push_back(std::move(entry));
	while (logEntries.size() > 500) {
		logEntries.pop_front();
	}
	++consoleStateVersion;
}

QString QMainController::createLogTimestamp() const {
	return QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
}

void QMainController::installQtMessageHandler() {
	gLogHandlerTarget.store(this, std::memory_order_release);
	gPreviousQtMessageHandler = qInstallMessageHandler(&QMainController::qtMessageHandler);
}

void QMainController::uninstallQtMessageHandler() {
	QMainController* expected = this;
	if (gLogHandlerTarget.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel)) {
		qInstallMessageHandler(gPreviousQtMessageHandler);
		gPreviousQtMessageHandler = nullptr;
	}
}

void QMainController::qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
	Q_UNUSED(context);

	if (QMainController* controller = gLogHandlerTarget.load(std::memory_order_acquire)) {
		controller->appendApplicationLog(message, qtMessageTypeToString(type));
	}

	if (gPreviousQtMessageHandler != nullptr) {
		gPreviousQtMessageHandler(type, context, message);
	}
}

QString QMainController::qtMessageTypeToString(QtMsgType type) {
	switch (type) {
		case QtDebugMsg:
			return QStringLiteral("debug");
		case QtInfoMsg:
			return QStringLiteral("info");
		case QtWarningMsg:
			return QStringLiteral("warning");
		case QtCriticalMsg:
			return QStringLiteral("critical");
		case QtFatalMsg:
			return QStringLiteral("fatal");
	}

	return QStringLiteral("app");
}

QString QMainController::executeLuaCommand(const QString& code, bool& success) {
	success = false;

	if (luaState == nullptr) {
		return QStringLiteral("Lua state is not available.");
	}

	const QString trimmedCode = code.trimmed();
	if (trimmedCode.isEmpty()) {
		return QStringLiteral("Input is empty.");
	}

	executionHistory.push_front(trimmedCode);
	while (executionHistory.size() > 100) {
		executionHistory.pop_back();
	}
	++consoleStateVersion;

	auto finalizeResult = [](const QString& tailMessage, const QString& emptyMessage) {
		return tailMessage.isEmpty() ? emptyMessage : tailMessage;
	};

	auto runChunk = [this](const QByteArray& sourceCode, QString& errorMessage, QString& resultText) {
		iris::iris_lua_t lua(luaState);
		auto loaded = lua.load(std::string_view(sourceCode.constData(), static_cast<size_t>(sourceCode.size())), "web-console");
		if (!loaded) {
			errorMessage = QString::fromStdString(loaded.message);
			return false;
		}

		const int baseTop = lua.native_get_top();
		auto callResult = lua.native_call(std::move(loaded.value()), 0);
		if (!callResult) {
			errorMessage = QString::fromStdString(callResult.message);
			lua.deref(std::move(loaded.value()));
			return false;
		}

		lua.deref(std::move(loaded.value()));

		QStringList resultValues;
		const int top = lua.native_get_top();
		for (int i = baseTop + 1; i <= top; ++i) {
			size_t len;
			const char* str = lua_tolstring(lua.get_state(), i, &len);
			resultValues.push_back(str == nullptr ? "" : QString::fromUtf8(str, len));
		}

		lua_settop(luaState, baseTop);
		if (!resultValues.isEmpty()) {
			resultText = QStringLiteral("=> %1").arg(resultValues.join(QStringLiteral("\t")));
		}

		return true;
	};

	const QByteArray utf8Code = trimmedCode.toUtf8();
	QString expressionError;
	QString statementError;
	QString resultText;
	if (!runChunk(QByteArray("return ") + utf8Code, expressionError, resultText) && !runChunk(utf8Code, statementError, resultText)) {
		lua_settop(luaState, 0);
		const QString result = finalizeResult(statementError.isEmpty() ? expressionError : statementError, QStringLiteral("Execution failed."));
		appendApplicationLog(result, QStringLiteral("error"));
		return result;
	}

	success = true;
	lua_settop(luaState, 0);
	const QString result = finalizeResult(resultText, QStringLiteral("Execution completed."));
	if (!result.isEmpty()) {
		appendApplicationLog(result, QStringLiteral("result"));
	}
	return result;
}
