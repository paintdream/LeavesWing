#pragma once

#include <QObject>
#include <QHash>

class QTcpServer;
class QTcpSocket;
class QMainController;

class QDebugConsoleServer : public QObject {
public:
	explicit QDebugConsoleServer(QMainController* mainController, QObject* parent = nullptr);
	bool startListening(quint16 port);

private:
	void handleNewConnection();
	void handleSocketReadyRead(QTcpSocket* socket);
	void handleSocketDisconnected(QTcpSocket* socket);
	void processRequest(QTcpSocket* socket, const QByteArray& requestData);
	void writeResponse(QTcpSocket* socket, const QByteArray& status, const QByteArray& contentType, const QByteArray& body);
	QByteArray buildConsolePage() const;
	QByteArray buildClearResponse() const;
	QByteArray buildExecuteResponse(const QByteArray& requestBody) const;
	QByteArray buildStateResponse() const;

private:
	QMainController* mainController;
	QTcpServer* server;
	QHash<QTcpSocket*, QByteArray> socketBuffers;
};
