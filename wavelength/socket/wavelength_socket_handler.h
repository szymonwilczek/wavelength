#ifndef WAVELENGTH_SOCKET_HANDLER_H
#define WAVELENGTH_SOCKET_HANDLER_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUrl>
#include <QDebug>

class WavelengthSocketHandler : public QObject {
    Q_OBJECT

public:
    static WavelengthSocketHandler* getInstance() {
        static WavelengthSocketHandler instance;
        return &instance;
    }

    QWebSocket* createSocket() {
        QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);
        connect(socket, &QWebSocket::connected, this, &WavelengthSocketHandler::onSocketConnected);
        connect(socket, &QWebSocket::disconnected, this, &WavelengthSocketHandler::onSocketDisconnected);
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), 
                this, &WavelengthSocketHandler::onSocketError);
        return socket;
    }

    bool connectSocket(QWebSocket* socket, const QString& url) {
        if (!socket) {
            qDebug() << "Cannot connect null socket";
            return false;
        }

        if (socket->state() == QAbstractSocket::ConnectedState) {
            qDebug() << "Socket already connected";
            return true;
        }

        socket->open(QUrl(url));
        return true;
    }

    void disconnectSocket(QWebSocket* socket) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->close();
        }
    }

    bool isSocketConnected(QWebSocket* socket) {
        return socket && socket->state() == QAbstractSocket::ConnectedState;
    }

    bool sendData(QWebSocket* socket, const QJsonObject& data) {
        if (!isSocketConnected(socket)) {
            qDebug() << "Cannot send data - socket not connected";
            return false;
        }

        QJsonDocument doc(data);
        socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
        return true;
    }

    void registerMessageHandler(QWebSocket* socket, const std::function<void(const QString&)>& handler) {
        if (!socket) return;
        connect(socket, &QWebSocket::textMessageReceived, this, handler);
    }

    void cleanupSocket(QWebSocket* socket) {
        if (!socket) return;
        socket->disconnect();
        socket->close();
        socket->deleteLater();
    }

signals:
    void socketConnected();
    void socketDisconnected();
    void socketError(const QString& errorMessage);
    void messageReceived(const QString& message);

private slots:
    void onSocketConnected() {
        qDebug() << "Socket connected";
        emit socketConnected();
    }

    void onSocketDisconnected() {
        qDebug() << "Socket disconnected";
        emit socketDisconnected();
    }

    void onSocketError(QAbstractSocket::SocketError error) {
        QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
        QString errorMsg = socket ? socket->errorString() : "Unknown error";
        qDebug() << "Socket error:" << error << "-" << errorMsg;
        emit socketError(errorMsg);
    }

private:
    WavelengthSocketHandler(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthSocketHandler() {}

    WavelengthSocketHandler(const WavelengthSocketHandler&) = delete;
    WavelengthSocketHandler& operator=(const WavelengthSocketHandler&) = delete;
};

#endif // WAVELENGTH_SOCKET_HANDLER_H