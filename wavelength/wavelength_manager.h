#ifndef WAVELENGTH_MANAGER_H
#define WAVELENGTH_MANAGER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QList>
#include <qpointer.h>
#include <QUuid>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <QWebSocket>
#include <QWebSocketServer>
#include <Qt>

struct WavelengthInfo {
    int frequency;
    QString name;
    bool isPasswordProtected;
    QString password;
    QString hostId;
    bool isHost;
    QString hostAddress;
    int hostPort;
    QPointer<QWebSocket> socket = nullptr;
    bool isClosing = false;
};

struct ClientInfo {
    QString clientId;
    QWebSocket* socket;
};

class WavelengthManager : public QObject {
    Q_OBJECT

public:

    const QString RELAY_SERVER_ADDRESS = "ws://localhost";
    const int RELAY_SERVER_PORT = 3000;

    static WavelengthManager* getInstance() {
        static WavelengthManager instance;
        return &instance;
    }

    bool isFrequencyAvailable(int frequency) {
        if (frequency < 30 || frequency > 300) {
            qDebug() << "Frequency out of valid range (30-300Hz)";
            return false;
        }

        try {
            auto collection = m_mongoClient["wavelengthDB"]["activeWavelengths"];
            auto document = bsoncxx::builder::basic::document{};
            document.append(bsoncxx::builder::basic::kvp("frequency", frequency));
            auto filter = document.view();
            auto result = collection.find_one(filter);

            bool available = !result;
            qDebug() << "Checking if frequency" << frequency << "is available:" << available;
            return available;
        }
        catch (const std::exception& e) {
            qDebug() << "MongoDB error:" << e.what();
            return false;
        }
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

    bool createWavelength(int frequency, const QString& name, bool isPasswordProtected,
                      const QString& password) {

    if (m_wavelengths.contains(frequency)) {
        qDebug() << "LOG: Częstotliwość" << frequency << "już istnieje lokalnie, ignoruję wywołanie";
        emit wavelengthCreated(frequency);
        return true;
    }

    if (!isFrequencyAvailable(frequency)) {
        qDebug() << "Cannot create wavelength - frequency" << frequency << "is already in use";
        return false;
    }

    QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);
    QString url = QString("%1:%2").arg(RELAY_SERVER_ADDRESS).arg(RELAY_SERVER_PORT);

    qDebug() << "Connecting to relay server at" << url;

    connect(socket, &QWebSocket::connected, this, [=]() {
        qDebug() << "Connected to relay server via WebSocket";

        QJsonObject registerMsg;
        registerMsg["type"] = "register_wavelength";
        registerMsg["frequency"] = frequency;
        registerMsg["name"] = name;
        registerMsg["isPasswordProtected"] = isPasswordProtected;

        QJsonDocument doc(registerMsg);
        socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

        qDebug() << "Sent wavelength registration request";
    });

    connect(socket, &QWebSocket::textMessageReceived, this, [=](const QString& message) {
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "Invalid JSON response from relay server";
            socket->close();
            emit connectionError("Invalid response from relay server");
            return;
        }

        QJsonObject response = doc.object();
        QString msgType = response["type"].toString();

        if (msgType == "register_result") {
            bool success = response["success"].toBool();

            if (!success) {
                qDebug() << "Failed to register wavelength:" << response["error"].toString();
                socket->close();
                emit connectionError(response["error"].toString());
                return;
            }

            QString sessionId = response["sessionId"].toString();
            qDebug() << "Wavelength registered with ID:" << sessionId;

            WavelengthInfo info;
            info.frequency = frequency;
            info.name = name.isEmpty() ? QString("Wavelength-%1").arg(frequency) : name;
            info.isPasswordProtected = isPasswordProtected;
            info.password = password;
            info.hostId = sessionId;
            info.isHost = true;
            info.hostAddress = RELAY_SERVER_ADDRESS;
            info.hostPort = RELAY_SERVER_PORT;
            info.socket = socket;

            m_wavelengths[frequency] = info;
            m_activeWavelength = frequency;

            qDebug() << "Successfully registered wavelength" << frequency;
            emit wavelengthCreated(frequency);
        }
        else if (msgType == "message") {
            QString sender = response["sender"].toString();
            QString content = response["content"].toString();

            QString formattedMessage = sender + ": " + content;
            emit messageReceived(frequency, formattedMessage);
        }
        else if (msgType == "client_joined") {
            QString clientId = response["clientId"].toString();
            qDebug() << "Client joined:" << clientId;
        }
        else if (msgType == "client_left") {
            QString clientId = response["clientId"].toString();
            qDebug() << "Client left:" << clientId;
        }
        else if (msgType == "wavelength_closed") {
            qDebug() << "Wavelength was closed:" << response["reason"].toString();

            if (m_wavelengths.contains(frequency)) {
                m_wavelengths.remove(frequency);

                if (m_activeWavelength == frequency) {
                    m_activeWavelength = -1;
                    emit wavelengthClosed(frequency);
                }
            }

            socket->close();
        }
    });

        connect(socket, &QWebSocket::disconnected, this, [this, frequency, socket]() {
        qDebug() << "Disconnected from relay server for wavelength" << frequency;

        if (m_wavelengths.contains(frequency) && m_wavelengths[frequency].socket == socket) {
            m_wavelengths[frequency].socket = nullptr;

            bool wasActive = (m_activeWavelength == frequency);
            if (wasActive) {
                m_activeWavelength = -1;
            }

            m_wavelengths.remove(frequency);

            if (wasActive) {
                emit wavelengthClosed(frequency);
            }
        }

    });

    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [=](QAbstractSocket::SocketError error) {
        qDebug() << "WebSocket error:" << error << "-" << socket->errorString();
        emit connectionError("Connection error: " + socket->errorString());

        socket->close();
        socket->deleteLater();
    });

    socket->open(QUrl(url));

    return true;
}

    bool joinWavelength(int frequency, const QString& password = QString()) {
    try {
        auto collection = m_mongoClient["wavelengthDB"]["activeWavelengths"];
        auto document = bsoncxx::builder::basic::document{};
        document.append(bsoncxx::builder::basic::kvp("frequency", frequency));
        auto filter = document.view();
        auto result = collection.find_one(filter);

        if (!result) {
            qDebug() << "Wavelength" << frequency << "does not exist";
            return false;
        }

        auto doc = result->view();
        bool isPasswordProtected = doc["isPasswordProtected"].get_bool().value;
        std::string nameStr = static_cast<std::string>(doc["name"].get_string().value);

        if (isPasswordProtected && password.isEmpty()) {
            qDebug() << "Password is required for this wavelength";
            return false;
        }

        QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);
        QString url = QString("%1:%2").arg(RELAY_SERVER_ADDRESS).arg(RELAY_SERVER_PORT);

        qDebug() << "Connecting to relay server at" << url << "to join wavelength" << frequency;

        connect(socket, &QWebSocket::connected, this, [=]() {
            qDebug() << "Connected to relay server via WebSocket";

            QJsonObject joinMsg;
            joinMsg["type"] = "join_wavelength";
            joinMsg["frequency"] = frequency;
            if (isPasswordProtected) {
                joinMsg["password"] = password;
            }

            QJsonDocument doc(joinMsg);
            socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            qDebug() << "Sent join request for wavelength" << frequency;
        });

        connect(socket, &QWebSocket::textMessageReceived, this, [=](const QString& message) {
            QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
            if (doc.isNull() || !doc.isObject()) {
                qDebug() << "Invalid JSON response from relay server";
                socket->close();
                emit connectionError("Invalid response from relay server");
                return;
            }

            QJsonObject response = doc.object();
            QString msgType = response["type"].toString();

            if (msgType == "join_result") {
                bool success = response["success"].toBool();

                if (!success) {
                    qDebug() << "Failed to join wavelength:" << response["error"].toString();
                    socket->close();
                    emit authenticationFailed(frequency);
                    return;
                }

                qDebug() << "Successfully joined wavelength" << frequency;
                emit wavelengthJoined(frequency);
            }
            else if (msgType == "message") {
                QString sender = response["sender"].toString();
                QString content = response["content"].toString();

                QString formattedMessage = sender + ": " + content;
                emit messageReceived(frequency, formattedMessage);
            }
            else if (msgType == "wavelength_closed") {
                qDebug() << "Wavelength was closed:" << response["reason"].toString();

                if (m_wavelengths.contains(frequency)) {
                    m_wavelengths.remove(frequency);

                    if (m_activeWavelength == frequency) {
                        m_activeWavelength = -1;
                        emit wavelengthClosed(frequency);
                    }
                }

                socket->close();
            }
        });

        connect(socket, &QWebSocket::disconnected, this, [=]() {
            qDebug() << "Disconnected from relay server";

            if (m_wavelengths.contains(frequency) && m_wavelengths[frequency].socket == socket) {
                m_wavelengths.remove(frequency);

                if (m_activeWavelength == frequency) {
                    m_activeWavelength = -1;
                    emit wavelengthClosed(frequency);
                }
            }

            socket->deleteLater();
        });

        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                this, [=](QAbstractSocket::SocketError error) {
            qDebug() << "WebSocket error:" << error << "-" << socket->errorString();
            emit connectionError("Connection error: " + socket->errorString());

            socket->close();
            socket->deleteLater();
        });

        WavelengthInfo info;
        info.frequency = frequency;
        info.name = QString::fromStdString(nameStr);
        info.isPasswordProtected = isPasswordProtected;
        info.password = password;
        info.isHost = false;
        info.hostAddress = RELAY_SERVER_ADDRESS;
        info.hostPort = RELAY_SERVER_PORT;
        info.socket = socket;

        m_wavelengths[frequency] = info;
        m_activeWavelength = frequency;

        socket->open(QUrl(url));

        return true;
    }
    catch (const std::exception& e) {
        qDebug() << "MongoDB error:" << e.what();
        return false;
    }
}

    void leaveWavelength() {
    qDebug() << "===== LEAVE SEQUENCE START (MANAGER) =====";

    if (m_activeWavelength == -1) {
        qDebug() << "Not connected to any wavelength";
        return;
    }

    int freq = m_activeWavelength;
    qDebug() << "Leaving wavelength" << freq;

    if (!m_wavelengths.contains(freq)) {
        m_activeWavelength = -1;
        qDebug() << "No active connection for wavelength" << freq;
        emit wavelengthLeft(freq);
        return;
    }

    auto& info = m_wavelengths[freq];
    qDebug() << "Found wavelength info, isHost =" << info.isHost;

    if (info.socket) {
        qDebug() << "Socket exists, valid =" << info.socket->isValid();

        if (info.socket->isValid()) {
            QJsonObject leaveMsg;
            if (info.isHost) {
                leaveMsg["type"] = "close_wavelength";
            } else {
                leaveMsg["type"] = "leave_wavelength";
            }
            leaveMsg["frequency"] = freq;

            QJsonDocument doc(leaveMsg);
            qDebug() << "Sending leave/close message to server";
            info.socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
        }

        qDebug() << "Disconnecting all signals from socket";
        info.socket->disconnect();

        qDebug() << "Closing socket";
        info.socket->close();

        QPointer<QWebSocket> socketPtr = info.socket;
        QTimer::singleShot(100, [socketPtr]() {
            qDebug() << "Delayed socket deletion (if still valid)";
            if (!socketPtr.isNull()) {
                socketPtr->deleteLater();
            }
        });

        info.socket = nullptr;
    }

    qDebug() << "Removing wavelength from map";
    m_wavelengths.remove(freq);
    m_activeWavelength = -1;

    qDebug() << "Left wavelength" << freq;
    qDebug() << "Emitting wavelengthLeft signal";
    emit wavelengthLeft(freq);
    qDebug() << "===== LEAVE SEQUENCE COMPLETE =====";
}

    bool getWavelengthInfo(int frequency, bool* isHost = nullptr) {
        if (!m_wavelengths.contains(frequency)) {
            return false;
        }

        if (isHost) {
            *isHost = m_wavelengths[frequency].isHost;
        }

        return true;
    }

    void closeWavelength(int frequency) {
    qDebug() << "===== CLOSE WAVELENGTH START =====";
    qDebug() << "Closing wavelength" << frequency;

    if (!m_wavelengths.contains(frequency)) {
        qDebug() << "Wavelength" << frequency << "does not exist";
        qDebug() << "===== CLOSE WAVELENGTH ABORTED =====";
        return;
    }

    if (!m_wavelengths[frequency].isHost) {
        qDebug() << "Cannot close wavelength" << frequency << "- not the host";
        qDebug() << "===== CLOSE WAVELENGTH ABORTED =====";
        return;
    }

    bool wasActive = (m_activeWavelength == frequency);
    qDebug() << "Was active wavelength:" << wasActive;

    m_wavelengths[frequency].isClosing = true;

    QPointer<QWebSocket> socketToClose = m_wavelengths[frequency].socket;
    m_wavelengths[frequency].socket = nullptr;

    qDebug() << "Socket pointer saved, valid:" << (socketToClose && socketToClose->isValid());

    if (m_activeWavelength == frequency) {
        m_activeWavelength = -1;
    }

    m_wavelengths.remove(frequency);
    qDebug() << "Removed wavelength from local map";

    if (socketToClose && socketToClose->isValid()) {
        qDebug() << "Sending close message to server";

        socketToClose->disconnect();

        QObject::connect(socketToClose, &QWebSocket::disconnected,
                        socketToClose, &QWebSocket::deleteLater);

        QJsonObject closeMsg;
        closeMsg["type"] = "close_wavelength";
        closeMsg["frequency"] = frequency;
        QJsonDocument doc(closeMsg);
        socketToClose->sendTextMessage(doc.toJson(QJsonDocument::Compact));
        qDebug() << "Close message sent to server";

        QTimer::singleShot(200, socketToClose, [socketToClose]() {
            qDebug() << "Delayed socket close triggered";
            if (!socketToClose.isNull()) {
                socketToClose->close();
            }
        });
    } else if (socketToClose) {
        qDebug() << "Socket exists but is invalid, scheduling deletion";
        socketToClose->deleteLater();
    }

    if (wasActive) {
        qDebug() << "Emitting wavelengthClosed signal";
        emit wavelengthClosed(frequency);
    }

    try {
        qDebug() << "Attempting to remove from MongoDB";
        auto collection = m_mongoClient["wavelengthDB"]["activeWavelengths"];
        auto document = bsoncxx::builder::basic::document{};
        document.append(bsoncxx::builder::basic::kvp("frequency", frequency));
        auto filter = document.view();
        collection.delete_one(filter);
        qDebug() << "Successfully removed from MongoDB";
    }
    catch (const std::exception& e) {
        qDebug() << "MongoDB error on delete:" << e.what();
    }

    qDebug() << "===== CLOSE WAVELENGTH COMPLETE =====";
}

    void sendMessage(const QString& message) {
        if (m_activeWavelength == -1) {
            qDebug() << "Not connected to any wavelength";
            return;
        }

        if (!m_wavelengths.contains(m_activeWavelength)) {
            qDebug() << "Wavelength not found in map";
            return;
        }

        QWebSocket* socket = m_wavelengths[m_activeWavelength].socket;

        if (!socket) {
            qDebug() << "Socket is null";
            return;
        }

        if (!socket->isValid()) {
            qDebug() << "Socket is not valid";
            return;
        }

        try {
            QString messageId = QUuid::createUuid().toString();

            QJsonObject msgObj;
            msgObj["type"] = "send_message";
            msgObj["message"] = message;
            msgObj["messageId"] = messageId;

            QJsonDocument doc(msgObj);
            QString jsonMessage = doc.toJson(QJsonDocument::Compact);

            m_processedMessageIds.insert(messageId);

            qDebug() << "Sending WebSocket message:" << jsonMessage;
            socket->sendTextMessage(jsonMessage);

            emit messageSent(m_activeWavelength, message);
        } catch (const std::exception& e) {
            qDebug() << "Exception in sendMessage:" << e.what();
        } catch (...) {
            qDebug() << "Unknown exception in sendMessage";
        }
    }

    int getActiveWavelength() const {
        return m_activeWavelength;
    }

signals:
    void wavelengthCreated(int frequency);
    void wavelengthJoined(int frequency);
    void wavelengthLeft(int frequency);
    void wavelengthClosed(int frequency);
    void messageSent(int frequency, const QString& message);
    void messageReceived(int frequency, const QString& message);
    void authenticationFailed(int frequency);
    void connectionError(const QString& errorMessage);

private slots:
    void handleNewConnection() {
        QWebSocket* clientSocket = m_wsServer->nextPendingConnection();

        if (!clientSocket) {
            return;
        }

        qDebug() << "New WebSocket client connected:" << clientSocket->peerAddress().toString();

        ClientInfo newClient;
        newClient.socket = clientSocket;
        newClient.clientId = "pending";
        m_pendingClients.append(newClient);

        connect(clientSocket, &QWebSocket::textMessageReceived,
                this, &WavelengthManager::handleServerMessage);
        connect(clientSocket, &QWebSocket::disconnected,
                this, &WavelengthManager::handleClientDisconnect);
    }

    void handleServerMessage(const QString& message) {
        QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
        if (!socket) return;

        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());

        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "Invalid message format";
            return;
        }

        QJsonObject msgObj = doc.object();
        QString msgType = msgObj["type"].toString();

        if (msgType == "auth") {
            handleClientAuth(socket, msgObj);
        } else if (msgType == "message") {
            if (m_activeWavelength != -1) {
                QString content = msgObj["content"].toString();
                QString senderId;

                for (const auto& client : m_clients) {
                    if (client.socket == socket) {
                        senderId = client.clientId.left(8);
                        break;
                    }
                }

                if (senderId.isEmpty()) {
                    senderId = "Unknown";
                }

                QString formattedMessage = senderId + ": " + content;
                emit messageReceived(m_activeWavelength, formattedMessage);

                QJsonObject forwardMsg;
                forwardMsg["type"] = "message";
                forwardMsg["sender"] = senderId;
                forwardMsg["content"] = content;

                QJsonDocument forwardDoc(forwardMsg);
                QString forwardData = forwardDoc.toJson(QJsonDocument::Compact);

                for (const auto& client : m_clients) {
                    if (client.socket && client.socket != socket) {
                        client.socket->sendTextMessage(forwardData);
                    }
                }
            }
        }
    }

    void handleClientMessage(const QString& message) {
        QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
        if (!socket || m_activeWavelength == -1) return;

    qDebug() << "Received client message:" << message;

        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());

        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "Invalid message format received from server";
            return;
        }

        QJsonObject msgObj = doc.object();
        QString msgType = msgObj["type"].toString();

    if (msgType == "message") {
        QString messageId;
        if (msgObj.contains("messageId")) {
            messageId = msgObj["messageId"].toString();

            if (!messageId.isEmpty() && m_processedMessageIds.contains(messageId)) {
                qDebug() << "Ignoring duplicate message with ID:" << messageId;
                return;
            }

            if (!messageId.isEmpty()) {
                m_processedMessageIds.insert(messageId);

                if (m_processedMessageIds.size() > 200) {
                    QList<QString> idsList = m_processedMessageIds.values();
                    for (int i = 0; i < idsList.size() / 2; i++) {
                        m_processedMessageIds.remove(idsList[i]);
                    }
                }
            }
        }

        QString sender = msgObj["sender"].toString();
        QString content = msgObj["content"].toString();
        bool isSelf = msgObj["isSelf"].toBool();

        qDebug() << "Message details - Sender:" << sender
                 << "Content:" << content
                 << "IsSelf:" << isSelf
                 << "MessageID:" << messageId;

        QString formattedMessage;
        if (isSelf) {
            formattedMessage = "You: " + content;
        } else {
            formattedMessage = sender + ": " + content;
        }

        qDebug() << "Emitting message:" << formattedMessage;
        emit messageReceived(m_activeWavelength, formattedMessage);
    } else if (msgType == "wavelength_closed") {
        qDebug() << "Server has shut down the wavelength";
        int freq = msgObj["frequency"].toInt();

        if (m_wavelengths.contains(freq)) {
            if (m_wavelengths[freq].socket) {
                m_wavelengths[freq].socket->disconnect();
                m_wavelengths[freq].socket->deleteLater();
            }

            m_wavelengths.remove(freq);

            if (m_activeWavelength == freq) {
                m_activeWavelength = -1;
                emit wavelengthClosed(freq);
            }
        }
    }
        else if (msgType == "serverShutdown") {
            qDebug() << "Server has shut down the wavelength";

            if (m_wavelengths.contains(m_activeWavelength)) {
                int freq = m_activeWavelength;

                if (m_wavelengths[freq].socket) {
                    m_wavelengths[freq].socket->disconnect();
                    m_wavelengths[freq].socket->deleteLater();
                }

                m_wavelengths.remove(freq);
                m_activeWavelength = -1;

                emit wavelengthClosed(freq);
            }
        }
        else if (msgType == "authResult") {
            bool success = msgObj["success"].toBool();
            if (!success) {
                QString reason = msgObj["reason"].toString();
                qDebug() << "Authentication failed:" << reason;

                socket->close();
                socket->deleteLater();

                if (m_wavelengths.contains(m_activeWavelength)) {
                    m_wavelengths.remove(m_activeWavelength);
                }

                m_activeWavelength = -1;

                emit authenticationFailed(m_activeWavelength);
            } else {
                emit wavelengthJoined(m_activeWavelength);
            }
        } else if (msgType == "close_confirmed") {
            qDebug() << "Received close confirmation from server";
            int freq = msgObj["frequency"].toInt();

            if (m_wavelengths.contains(freq) && !m_wavelengths[freq].isClosing) {
                if (m_wavelengths[freq].socket) {
                    m_wavelengths[freq].socket->disconnect();
                    m_wavelengths[freq].socket->deleteLater();
                }

                m_wavelengths.remove(freq);

                if (m_activeWavelength == freq) {
                    m_activeWavelength = -1;
                    emit wavelengthClosed(freq);
                }
            }
        }
    }

    void handleClientAuth(QWebSocket* socket, const QJsonObject& msgObj) {
        QString password = msgObj["password"].toString();
        QString clientId = msgObj["clientId"].toString();

        bool authenticated = false;
        QString reason;

        if (m_activeWavelength != -1 && m_wavelengths.contains(m_activeWavelength)) {
            const auto& info = m_wavelengths[m_activeWavelength];

            if (info.isPasswordProtected && password != info.password) {
                reason = "Invalid password";
            } else {
                authenticated = true;
            }
        } else {
            reason = "Wavelength not active";
        }

        QJsonObject response;
        response["type"] = "authResult";
        response["success"] = authenticated;

        if (!authenticated) {
            response["reason"] = reason;

            QJsonDocument doc(response);
            socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
            socket->close();
        } else {
            QJsonDocument doc(response);
            socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            for (int i = 0; i < m_pendingClients.size(); ++i) {
                if (m_pendingClients[i].socket == socket) {
                    ClientInfo client = m_pendingClients[i];
                    client.clientId = clientId;
                    m_clients.append(client);
                    m_pendingClients.removeAt(i);
                    break;
                }
            }

            qDebug() << "Client" << clientId << "authenticated successfully";
        }
    }

    void handleClientDisconnect() {
        QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
        if (!socket) return;

        qDebug() << "WebSocket client disconnected";

        for (int i = 0; i < m_pendingClients.size(); ++i) {
            if (m_pendingClients[i].socket == socket) {
                m_pendingClients.removeAt(i);
                break;
            }
        }

        for (int i = 0; i < m_clients.size(); ++i) {
            if (m_clients[i].socket == socket) {
                m_clients.removeAt(i);
                break;
            }
        }

        socket->deleteLater();
    }

    void handleServerDisconnect() {
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    qDebug() << "Disconnected from WebSocket server";

    QList<int> frequenciesToRemove;
    for (auto it = m_wavelengths.begin(); it != m_wavelengths.end(); ++it) {
        if (it.value().socket == socket) {
            frequenciesToRemove.append(it.key());
        }
    }

    foreach (int freq, frequenciesToRemove) {
        bool wasActive = (m_activeWavelength == freq);

        if (m_wavelengths.contains(freq)) {
            m_wavelengths[freq].socket = nullptr;
        }

        m_wavelengths.remove(freq);

        if (wasActive) {
            m_activeWavelength = -1;
            emit wavelengthClosed(freq);
        }
    }

}

private:
    WavelengthManager(QObject* parent = nullptr) : QObject(parent), m_activeWavelength(-1), m_wsServer(nullptr) {
        try {
            static mongocxx::instance instance{};
            m_mongoClient = mongocxx::client{mongocxx::uri{"mongodb+srv://wolfiksw:asdasdas@testcluster.taymr.mongodb.net/"}};

            qDebug() << "Connected to MongoDB successfully";
        }
        catch (const std::exception& e) {
            qDebug() << "Failed to connect to MongoDB:" << e.what();
        }
    }

    ~WavelengthManager() {
        for (auto it = m_wavelengths.begin(); it != m_wavelengths.end(); ++it) {
            if (it.value().isHost) {
                closeWavelength(it.key());
            }
        }

        if (m_wsServer) {
            m_wsServer->close();
            delete m_wsServer;
        }
    }

    WavelengthManager(const WavelengthManager&) = delete;
    WavelengthManager& operator=(const WavelengthManager&) = delete;

    QMap<int, WavelengthInfo> m_wavelengths;
    int m_activeWavelength;
    QWebSocketServer* m_wsServer;
    QList<ClientInfo> m_clients;
    QList<ClientInfo> m_pendingClients;
    mongocxx::client m_mongoClient;
    bool m_closing = false;
    QMetaObject::Connection m_closeMessageConnection;
    QSet<QString> m_processedMessageIds;
};

#endif // WAVELENGTH_MANAGER_H