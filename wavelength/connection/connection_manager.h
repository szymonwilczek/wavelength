#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <QObject>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QTimer>
#include <QPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QUrl>
#include <QDebug>

class ConnectionManager : public QObject {
    Q_OBJECT

public:
    static ConnectionManager* getInstance() {
        static ConnectionManager instance;
        return &instance;
    }

    QString getLocalIpAddress() {
        foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol &&
                !address.isLoopback() &&
                address.toString().startsWith("192.168.")) {
                return address.toString();
            }
        }
        return "127.0.0.1";
    }

    QWebSocket* connectToRelayServer(const QString& serverAddress, int serverPort, 
                                    const std::function<void(bool)>& onConnected,
                                    const std::function<void(const QString&)>& onTextMessageReceived,
                                    const std::function<void()>& onDisconnected) {
        QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

        // Poprawiony format URL dla WebSocket
        QString url;
        if (!serverAddress.startsWith("ws://") && !serverAddress.startsWith("wss://")) {
            url = QString("ws://%1:%2").arg(serverAddress).arg(serverPort);
        } else {
            // Jeśli adres już zawiera protokół, usuń tylko część z protokołem
            QString hostPart = serverAddress;
            if (serverAddress.startsWith("ws://")) {
                hostPart = serverAddress.mid(5); // Usuń "ws://"
            } else if (serverAddress.startsWith("wss://")) {
                hostPart = serverAddress.mid(6); // Usuń "wss://"
            }
            url = QString("ws://%1:%2").arg(hostPart).arg(serverPort);
        }

        qDebug() << "Connecting to relay server at" << url;

        connect(socket, &QWebSocket::connected, this, [this, socket, onConnected]() {
            qDebug() << "Connected to relay server";
            m_activeSockets.append(socket);
            onConnected(true);
        });

        connect(socket, &QWebSocket::textMessageReceived, this, [onTextMessageReceived](const QString& message) {
            qDebug() << "Received message from server:" << message;
            onTextMessageReceived(message);
        });

        connect(socket, &QWebSocket::disconnected, this, [this, socket, onDisconnected]() {
            qDebug() << "Disconnected from relay server with reason:" << socket->closeReason();
            m_activeSockets.removeOne(socket);
            onDisconnected();
            socket->deleteLater();
        });

        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this, socket, onConnected](QAbstractSocket::SocketError error) {
                qDebug() << "WebSocket error:" << error << "-" << socket->errorString();
                m_activeSockets.removeOne(socket);
                onConnected(false);
                socket->deleteLater();
            });

        QUrl wsUrl(url);
        qDebug() << "Opening WebSocket connection to URL:" << wsUrl.toString();
        socket->open(wsUrl);
        return socket;
    }

    void disconnectFromServer(QPointer<QWebSocket> socket, bool graceful = true) {
        if (socket.isNull()) {
            return;
        }

        qDebug() << "Disconnecting from server" << (graceful ? "gracefully" : "immediately");

        if (graceful) {
            socket->close();
        } else {
            socket->abort();
        }

        QTimer::singleShot(100, [socket]() {
            if (!socket.isNull()) {
                socket->deleteLater();
            }
        });

        m_activeSockets.removeOne(socket.data());
    }

    bool startServer(int port) {
        if (m_server) {
            qDebug() << "Server already running";
            return false;
        }

        m_server = new QWebSocketServer("Wavelength Server", QWebSocketServer::NonSecureMode, this);

        if (!m_server->listen(QHostAddress::Any, port)) {
            qDebug() << "Failed to start WebSocket server:" << m_server->errorString();
            delete m_server;
            m_server = nullptr;
            return false;
        }

        qDebug() << "WebSocket server started on port" << port;

        connect(m_server, &QWebSocketServer::newConnection,
                this, &ConnectionManager::onNewConnection);

        return true;
    }

    void stopServer() {
        if (!m_server) {
            return;
        }

        for (QWebSocket* client : m_serverClients) {
            client->close();
        }

        qDeleteAll(m_serverClients);
        m_serverClients.clear();

        m_server->close();
        delete m_server;
        m_server = nullptr;

        qDebug() << "WebSocket server stopped";
    }

    QJsonDocument createJsonMessage(const QString& type, const QJsonObject& data = QJsonObject()) {
        QJsonObject message;
        message["type"] = type;

        for (auto it = data.begin(); it != data.end(); ++it) {
            message[it.key()] = it.value();
        }

        return QJsonDocument(message);
    }

    void sendJsonMessage(QWebSocket* socket, const QJsonDocument& doc) {
        if (!socket || !socket->isValid()) {
            qDebug() << "Cannot send message - socket is invalid";
            return;
        }

        QString message = doc.toJson(QJsonDocument::Compact);
        qDebug() << "Sending message to server:" << message;
        socket->sendTextMessage(message);
    }

signals:
    void newClientConnected(QWebSocket* socket);
    void clientDisconnected(QWebSocket* socket);
    void serverMessageReceived(QWebSocket* socket, const QString& message);

private slots:
    void onNewConnection() {
        QWebSocket* client = m_server->nextPendingConnection();

        if (!client) {
            return;
        }

        qDebug() << "New client connected from" << client->peerAddress().toString();
        m_serverClients.append(client);

        connect(client, &QWebSocket::textMessageReceived, this,
                [this, client](const QString& message) {
                    qDebug() << "Received message from client:" << message;
                    emit serverMessageReceived(client, message);
                });

        connect(client, &QWebSocket::disconnected, this,
                [this, client]() {
                    qDebug() << "Client disconnected with reason:" << client->closeReason();
                    m_serverClients.removeOne(client);
                    emit clientDisconnected(client);
                    client->deleteLater();
                });
                
        emit newClientConnected(client);
    }

private:
    ConnectionManager(QObject* parent = nullptr) : QObject(parent), m_server(nullptr) {}
    ~ConnectionManager() {
        stopServer();
        
        for (QWebSocket* socket : m_activeSockets) {
            socket->abort();
            socket->deleteLater();
        }
        m_activeSockets.clear();
    }
    
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    QList<QWebSocket*> m_activeSockets;
    QList<QWebSocket*> m_serverClients;
    QWebSocketServer* m_server;
};

#endif // CONNECTION_MANAGER_H