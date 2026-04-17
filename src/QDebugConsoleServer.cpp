#include "QDebugConsoleServer.h"
#include "QMainController.h"
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringBuilder>
#include <QTcpServer>
#include <QTcpSocket>

QDebugConsoleServer::QDebugConsoleServer(QMainController* mainController, QObject* parent)
	: QObject(parent), mainController(mainController), server(new QTcpServer(this)) {
	connect(server, &QTcpServer::newConnection, this, &QDebugConsoleServer::handleNewConnection);
}

bool QDebugConsoleServer::startListening(quint16 port) {
	return server->listen(QHostAddress::Any, port);
}

void QDebugConsoleServer::handleNewConnection() {
	while (QTcpSocket* socket = server->nextPendingConnection()) {
		connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
			handleSocketReadyRead(socket);
		});
		connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
			handleSocketDisconnected(socket);
			socket->deleteLater();
		});
	}
}

void QDebugConsoleServer::handleSocketReadyRead(QTcpSocket* socket) {
	QByteArray& buffer = socketBuffers[socket];
	buffer.append(socket->readAll());

	const qsizetype headerEnd = buffer.indexOf("\r\n\r\n");
	if (headerEnd < 0) {
		return;
	}

	const QByteArray headers = buffer.left(headerEnd);
	const QByteArray body = buffer.mid(headerEnd + 4);
	qsizetype contentLength = 0;
	const QList<QByteArray> headerLines = headers.split('\n');
	for (const QByteArray& rawLine : headerLines) {
		const QByteArray line = rawLine.trimmed();
		if (line.startsWith("Content-Length:")) {
			bool ok = false;
			contentLength = line.mid(sizeof("Content-Length:") - 1).trimmed().toLongLong(&ok);
			if (!ok) {
				contentLength = 0;
			}
		}
	}

	if (body.size() < contentLength) {
		return;
	}

	processRequest(socket, buffer.left(headerEnd + 4 + contentLength));
	socketBuffers.remove(socket);
}

void QDebugConsoleServer::handleSocketDisconnected(QTcpSocket* socket) {
	socketBuffers.remove(socket);
}

void QDebugConsoleServer::processRequest(QTcpSocket* socket, const QByteArray& requestData) {
	const qsizetype headerEnd = requestData.indexOf("\r\n\r\n");
	if (headerEnd < 0) {
		writeResponse(socket, "400 Bad Request", "text/plain; charset=utf-8", "Bad Request");
		return;
	}

	const QByteArray headerBlock = requestData.left(headerEnd);
	const QByteArray body = requestData.mid(headerEnd + 4);
	const QList<QByteArray> requestLines = headerBlock.split('\n');
	if (requestLines.isEmpty()) {
		writeResponse(socket, "400 Bad Request", "text/plain; charset=utf-8", "Bad Request");
		return;
	}

	const QList<QByteArray> requestParts = requestLines.front().trimmed().split(' ');
	if (requestParts.size() < 2) {
		writeResponse(socket, "400 Bad Request", "text/plain; charset=utf-8", "Bad Request");
		return;
	}

	const QByteArray method = requestParts[0];
	const QByteArray path = requestParts[1];
	if (method == "GET" && path == "/") {
		writeResponse(socket, "200 OK", "text/html; charset=utf-8", buildConsolePage());
	} else if (method == "GET" && path == "/api/state") {
		writeResponse(socket, "200 OK", "application/json; charset=utf-8", buildStateResponse());
	} else if (method == "POST" && path == "/api/clear") {
		writeResponse(socket, "200 OK", "application/json; charset=utf-8", buildClearResponse());
	} else if (method == "POST" && path == "/api/execute") {
		writeResponse(socket, "200 OK", "application/json; charset=utf-8", buildExecuteResponse(body));
	} else {
		writeResponse(socket, "404 Not Found", "text/plain; charset=utf-8", "Not Found");
	}
}

void QDebugConsoleServer::writeResponse(QTcpSocket* socket, const QByteArray& status, const QByteArray& contentType, const QByteArray& body) {
	QByteArray response;
	response += "HTTP/1.1 " + status + "\r\n";
	response += "Content-Type: " + contentType + "\r\n";
	response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
	response += "Connection: close\r\n";
	response += "Cache-Control: no-store\r\n";
	response += "\r\n";
	response += body;

	socket->write(response);
	socket->disconnectFromHost();
}

QByteArray QDebugConsoleServer::buildConsolePage() const {
	return QStringLiteral(R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>LeavesWing Debug Console</title>
<style>
    :root {
        color-scheme: light;
        --primary: #7C4DFF;
        --primary-dark: #5E35B1;
        --surface: #FFFFFF;
        --surface-alt: #F3EEFF;
        --background: #FAFAFA;
        --border: #E0E0E0;
        --text: #212121;
        --muted: #757575;
    }
    body {
        margin: 0;
        font-family: "Segoe UI", sans-serif;
        background: var(--background);
        color: var(--text);
        min-height: 100vh;
        display: grid;
        grid-template-rows: auto 1fr auto;
    }
    header {
        background: var(--primary);
        color: white;
        padding: 16px 24px;
        box-shadow: 0 2px 8px rgba(0, 0, 0, 0.12);
    }
    header h1 {
        margin: 0;
        font-size: 20px;
    }
    header p {
        margin: 6px 0 0;
        opacity: 0.92;
    }
    main {
        padding: 16px;
        display: grid;
        grid-template-columns: minmax(0, 1fr) 280px;
        gap: 16px;
       align-content: stretch;
    }
    .panel {
        background: var(--surface);
        border: 1px solid var(--border);
        border-radius: 16px;
        padding: 16px;
        box-shadow: 0 4px 14px rgba(124, 77, 255, 0.08);
    }
    .panel h2 {
        margin: 0 0 8px;
        font-size: 16px;
        color: var(--primary-dark);
    }
   .panel-header {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 12px;
        margin-bottom: 12px;
    }
    .console-panel {
        background: linear-gradient(180deg, var(--surface-alt), var(--surface));
    }
    #output {
        min-height: 360px;
        height: calc(100vh - 280px);
        overflow: auto;
        background: #1E1E1E;
        color: #F5F5F5;
        border-radius: 12px;
      padding: 0;
        font: 14px/1.5 Consolas, "Courier New", monospace;
    }
  .log-entry {
        display: grid;
        grid-template-columns: 96px 88px minmax(0, 1fr);
        gap: 12px;
        padding: 10px 16px;
        border-bottom: 1px solid rgba(255, 255, 255, 0.06);
        align-items: start;
    }
    .log-entry:last-child {
        border-bottom: none;
    }
    .log-time,
    .log-category {
        color: #A7A7A7;
        user-select: none;
    }
    .log-category {
        text-transform: uppercase;
    }
    .log-message {
        white-space: pre-wrap;
        word-break: break-word;
        color: #F5F5F5;
        user-select: text;
    }
    #history {
        list-style: none;
        margin: 0;
        padding: 0;
        display: grid;
        gap: 8px;
        max-height: calc(100vh - 280px);
        overflow: auto;
    }
    .history-item {
        border-radius: 10px;
        background: #F6F1FF;
        border: 1px solid #E5D8FF;
        padding: 10px 12px;
        font: 13px/1.5 Consolas, "Courier New", monospace;
        white-space: pre-wrap;
        word-break: break-word;
        cursor: pointer;
    }
    .history-item:hover {
        border-color: var(--primary);
    }
    footer {
        position: sticky;
        bottom: 0;
        background: rgba(250, 250, 250, 0.96);
        backdrop-filter: blur(10px);
        border-top: 1px solid var(--border);
        padding: 16px;
    }
    .form-grid {
        display: grid;
        grid-template-columns: 1fr auto;
        gap: 12px;
        align-items: end;
    }
    textarea {
        width: 100%;
        min-height: 92px;
        max-height: 220px;
        resize: vertical;
        border-radius: 12px;
        border: 1px solid var(--border);
        padding: 12px 14px;
        font: 14px/1.5 Consolas, "Courier New", monospace;
        box-sizing: border-box;
    }
    button {
        border: none;
        border-radius: 12px;
        padding: 12px 20px;
        background: var(--primary);
        color: white;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        min-width: 120px;
        height: 48px;
    }
    button:disabled {
        opacity: 0.6;
        cursor: wait;
    }
    .small {
        margin-top: 8px;
        font-size: 12px;
        color: var(--muted);
    }
 .actions {
        display: flex;
        gap: 8px;
        flex-wrap: wrap;
    }
    .secondary {
        background: white;
        color: var(--primary-dark);
        border: 1px solid #D1C4E9;
    }
    .status {
        font-size: 12px;
        color: var(--muted);
    }
    @media (max-width: 640px) {
        main {
            grid-template-columns: 1fr;
        }
        .form-grid {
            grid-template-columns: 1fr;
        }
        button {
            width: 100%;
        }
       #output,
        #history {
            height: auto;
            max-height: none;
        }
       .log-entry {
            grid-template-columns: 1fr;
            gap: 4px;
        }
    }
</style>
</head>
<body>
    <header>
        <h1>LeavesWing Debug Console</h1>
        <p>Live Lua execution, history, and auto-refresh application logs on port 8068.</p>
    </header>
    <main>
        <section class="panel console-panel">
         <div class="panel-header">
                <h2>Console Output</h2>
                <div class="actions">
                  <button id="copyMessagesButton" class="secondary" type="button">Copy Messages</button>
                    <button id="toggleRefreshButton" class="secondary" type="button">Pause Refresh</button>
                    <button id="clearButton" class="secondary" type="button">Clear Output</button>
                </div>
            </div>
          <div id="output"></div>
        </section>
     <aside class="panel">
            <div class="panel-header">
                <h2>Execution History</h2>
                <span id="refreshStatus" class="status">Auto refresh on</span>
            </div>
            <ul id="history"></ul>
        </aside>
    </main>
    <footer>
        <div class="form-grid">
            <textarea id="code" spellcheck="false" placeholder="Enter Lua code here. Use Ctrl+Enter to run."></textarea>
            <button id="runButton" type="button">Run Lua</button>
        </div>
        <div class="small">Examples: <code>print(&quot;hello&quot;)</code>, <code>return 1 + 2</code>, <code>test = (test or 0) + 1</code></div>
    </footer>
    <script>
        const output = document.getElementById('output');
        const code = document.getElementById('code');
        const history = document.getElementById('history');
     const copyMessagesButton = document.getElementById('copyMessagesButton');
        const toggleRefreshButton = document.getElementById('toggleRefreshButton');
        const clearButton = document.getElementById('clearButton');
        const refreshStatus = document.getElementById('refreshStatus');
        const runButton = document.getElementById('runButton');
        let stateVersion = -1;
        let lastLogId = 0;
        let refreshTimer = null;
        let isAutoRefreshEnabled = true;

        function isOutputSelectionActive() {
            const selection = window.getSelection();
            if (!selection || selection.rangeCount === 0 || selection.isCollapsed) {
                return false;
            }

            const range = selection.getRangeAt(0);
            return output.contains(range.commonAncestorContainer);
        }

        function createLogEntryElement(entry) {
            const row = document.createElement('div');
            row.className = 'log-entry';
            row.dataset.logId = String(entry.id || 0);

            const time = document.createElement('div');
            time.className = 'log-time';
            time.textContent = entry.timestamp || '';

            const category = document.createElement('div');
            category.className = 'log-category';
            category.textContent = entry.category || '';

            const message = document.createElement('div');
            message.className = 'log-message';
            message.textContent = entry.message || '';

            row.append(time, category, message);
            return row;
        }

        function normalizeLogEntry(entry) {
            if (!entry || typeof entry !== 'object' || Array.isArray(entry)) {
                return {
                    id: 0,
                    timestamp: '',
                    category: 'APP',
                    message: String(entry ?? '')
                };
            }

            return {
                id: Number(entry.id || 0),
                timestamp: typeof entry.timestamp === 'string' ? entry.timestamp : String(entry.timestamp ?? ''),
                category: typeof entry.category === 'string' ? entry.category : String(entry.category ?? 'APP'),
                message: typeof entry.message === 'string' ? entry.message : String(entry.message ?? '')
            };
        }

        function renderPlaceholder() {
            output.innerHTML = '';
            output.appendChild(createLogEntryElement({
                id: 0,
                timestamp: '',
                category: 'READY',
                message: 'LeavesWing debug console ready.'
            }));
        }

        function getVisibleLogMessages() {
            return Array.from(output.querySelectorAll('.log-message')).map(element => element.textContent || '').filter(text => text.length > 0);
        }

        function updateRefreshStatus(text) {
            refreshStatus.textContent = text;
        }

        function updateRefreshToggle() {
            toggleRefreshButton.textContent = isAutoRefreshEnabled ? 'Pause Refresh' : 'Resume Refresh';
            updateRefreshStatus(isAutoRefreshEnabled ? 'Auto refresh on' : 'Auto refresh paused');
        }

      function renderState(payload) {
            const logs = Array.isArray(payload.logs) ? payload.logs.map(normalizeLogEntry) : [];
            const entries = Array.isArray(payload.history) ? payload.history : [];
         const hasActiveSelection = isOutputSelectionActive();
            const shouldAppendOnly = !hasActiveSelection && logs.length > 0 && payload.version === stateVersion && lastLogId > 0;

            if (logs.length === 0) {
              renderPlaceholder();
                lastLogId = 0;
            } else if (shouldAppendOnly) {
                logs.filter(entry => (entry.id || 0) > lastLogId).forEach(entry => {
                    output.appendChild(createLogEntryElement(entry));
                    lastLogId = Math.max(lastLogId, entry.id || 0);
                });
            } else if (!hasActiveSelection) {
                output.innerHTML = '';
                logs.forEach(entry => {
                    output.appendChild(createLogEntryElement(entry));
                    lastLogId = Math.max(lastLogId, entry.id || 0);
                });
            }

            if (!hasActiveSelection) {
                if (logs.length === 0) {
                    lastLogId = 0;
                }
                output.scrollTop = output.scrollHeight;
            }

            history.innerHTML = '';
            entries.forEach(item => {
                const li = document.createElement('li');
                li.className = 'history-item';
                li.textContent = item;
                li.addEventListener('click', () => {
                    code.value = item;
                    code.focus();
                });
                history.appendChild(li);
            });

            stateVersion = typeof payload.version === 'number' ? payload.version : stateVersion;
        }

     async function refreshState(force = false) {
            if (!force && !isAutoRefreshEnabled) {
                return;
            }

            try {
                const response = await fetch('/api/state', { cache: 'no-store' });
                const payload = await response.json();
               if (typeof payload.version !== 'number' || payload.version !== stateVersion || !isOutputSelectionActive()) {
                    renderState(payload);
                }
              if (isAutoRefreshEnabled) {
                    updateRefreshStatus('Auto refresh on');
                }
            } catch (error) {
                updateRefreshStatus(isAutoRefreshEnabled ? 'Auto refresh retrying' : 'Auto refresh paused');
            }
        }

        async function copyMessages() {
            const text = getVisibleLogMessages().join('\n');
            if (!text) {
                updateRefreshStatus(isAutoRefreshEnabled ? 'No log messages to copy' : 'Auto refresh paused');
                return;
            }

            copyMessagesButton.disabled = true;
            try {
                await navigator.clipboard.writeText(text);
                updateRefreshStatus('Copied log messages');
            } catch (error) {
                updateRefreshStatus('Copy failed');
            } finally {
                copyMessagesButton.disabled = false;
            }
        }

        async function toggleAutoRefresh() {
            isAutoRefreshEnabled = !isAutoRefreshEnabled;
            updateRefreshToggle();

            if (isAutoRefreshEnabled) {
                await refreshState(true);
            }
        }

        async function runCode() {
            const source = code.value.trim();
            if (!source) {
               refreshStatus.textContent = 'Input is empty';
                return;
            }

            runButton.disabled = true;

            try {
              await fetch('/api/execute', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ code: source })
                });
             await refreshState(true);
            } catch (error) {
               updateRefreshStatus(error.message || 'Request failed');
            } finally {
                runButton.disabled = false;
            }
        }

        async function clearOutput() {
            clearButton.disabled = true;
            try {
                await fetch('/api/clear', { method: 'POST' });
               await refreshState(true);
            } finally {
                clearButton.disabled = false;
            }
        }

       copyMessagesButton.addEventListener('click', copyMessages);
        toggleRefreshButton.addEventListener('click', toggleAutoRefresh);
        runButton.addEventListener('click', runCode);
     clearButton.addEventListener('click', clearOutput);
        code.addEventListener('keydown', event => {
            if (event.key === 'Enter' && event.ctrlKey) {
                event.preventDefault();
                runCode();
            }
        });

     updateRefreshToggle();
        refreshState(true);
        refreshTimer = setInterval(refreshState, 1000);
       renderPlaceholder();
    </script>
</body>
</html>)HTML").toUtf8();
}

QByteArray QDebugConsoleServer::buildClearResponse() const {
	if (mainController != nullptr) {
		mainController->clearConsoleOutput();
	}

	return QJsonDocument(QJsonObject {
		{ QStringLiteral("success"), true }
		}).toJson(QJsonDocument::Compact);
}

QByteArray QDebugConsoleServer::buildExecuteResponse(const QByteArray& requestBody) const {
	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(requestBody, &parseError);
	if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
		return QJsonDocument(QJsonObject {
			{ QStringLiteral("success"), false },
			{ QStringLiteral("output"), QStringLiteral("Invalid request payload.") }
			}).toJson(QJsonDocument::Compact);
	}

	const QString code = document.object().value(QStringLiteral("code")).toString();
	bool success = false;
	const QString output = mainController != nullptr
		? mainController->executeLuaCommand(code, success)
		: QStringLiteral("Main controller is not available.");

	return QJsonDocument(QJsonObject {
		{ QStringLiteral("success"), success },
		{ QStringLiteral("output"), output }
		}).toJson(QJsonDocument::Compact);
}

QByteArray QDebugConsoleServer::buildStateResponse() const {
	QJsonArray logs;
	QJsonArray history;
	quint64 version = 0;

	if (mainController != nullptr) {
		const QList<DebugConsoleLogEntry> logSnapshot = mainController->getConsoleLogSnapshot();
		for (const DebugConsoleLogEntry& entry : logSnapshot) {
			logs.append(QJsonObject {
				{ QStringLiteral("id"), static_cast<qint64>(entry.id) },
				{ QStringLiteral("timestamp"), entry.timestamp },
				{ QStringLiteral("category"), entry.category },
				{ QStringLiteral("message"), entry.message }
				});
		}

		const QStringList historySnapshot = mainController->getExecutionHistorySnapshot();
		for (const QString& entry : historySnapshot) {
			history.append(entry);
		}

		version = mainController->getConsoleStateVersion();
	}

	return QJsonDocument(QJsonObject {
	   { QStringLiteral("version"), static_cast<qint64>(version) },
		{ QStringLiteral("logs"), logs },
		{ QStringLiteral("history"), history }
		}).toJson(QJsonDocument::Compact);
}
