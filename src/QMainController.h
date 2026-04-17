#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <utility>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include "../ref/iris/src/iris_dispatcher.h"
#include "../ref/iris/src/iris_lua.h"

using AsyncWorker = iris::iris_async_worker_t<>;
using Warp = iris::iris_warp_t<AsyncWorker>;

struct DebugConsoleLogEntry {
	quint64 id = 0;
	QString timestamp;
	QString category;
	QString message;
};

struct lua_State;
class QDebugConsoleServer;

class QMainController : public QObject {
public:
	explicit QMainController(QObject* parent = nullptr);
	~QMainController() override;

	iris::iris_async_worker_t<>& getAsyncWorker() noexcept {
		return asyncWorker;
	}

	void appendApplicationLog(const QString& message, const QString& category = QStringLiteral("app"));
	void clearConsoleOutput();
	QString executeLuaCommand(const QString& code, bool& success);
	QList<DebugConsoleLogEntry> getConsoleLogSnapshot() const;
	QStringList getExecutionHistorySnapshot() const;
	quint64 getConsoleStateVersion() const;

	template <typename F>
	void invokeMain(F&& f) {
		mainWarp.queue_routine(std::forward<F>(f));
		flushMain();
	}

private:
	void flushMain();
	void initializeLuaState();
	void shutdownLuaState();
	void appendConsoleLogLocked(const QString& category, const QString& message);
	QString createLogTimestamp() const;
	void installQtMessageHandler();
	void uninstallQtMessageHandler();
	void luaPrint(iris::iris_lua_t lua, iris::iris_lua_t::stackindex_t);
	static void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
	static QString qtMessageTypeToString(QtMsgType type);

private:
	AsyncWorker asyncWorker;
	Warp mainWarp;
	std::atomic<Warp::queue_state_t> queueState = Warp::queue_state_t::idle;
	std::unique_ptr<QDebugConsoleServer> debugConsoleServer;
	lua_State* luaState = nullptr;
	mutable std::mutex consoleMutex;
	QList<DebugConsoleLogEntry> logEntries;
	QStringList executionHistory;
	quint64 nextLogEntryId = 1;
	quint64 consoleStateVersion = 0;
	static std::atomic<QMainController*> gLogHandlerTarget;
	static QtMessageHandler gPreviousQtMessageHandler;
};
