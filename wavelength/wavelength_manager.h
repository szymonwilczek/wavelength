#ifndef WAVELENGTH_MANAGER_H
#define WAVELENGTH_MANAGER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QList>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

struct WavelengthInfo {
    int frequency;
    QString name;
    bool isPasswordProtected;
    QString password;
    QString hostId;
    bool isHost;
    QString hostAddress;
    int hostPort;
    QTcpSocket* socket = nullptr;
};

struct ClientInfo {
    QString clientId;
    QTcpSocket* socket;
};

class WavelengthManager : public QObject {
    Q_OBJECT

public:
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

        if (m_tcpServer) {
            delete m_tcpServer;
        }
        m_tcpServer = new QTcpServer(this);

        if (!m_tcpServer->listen(QHostAddress::Any, 0)) {
            qDebug() << "Failed to start TCP server:" << m_tcpServer->errorString();
            return false;
        }

        int port = m_tcpServer->serverPort();
        QString hostAddress = getLocalIpAddress();
        qDebug() << "Server started on" << hostAddress << "port" << port;

        try {
            auto collection = m_mongoClient["wavelengthDB"]["activeWavelengths"];

            auto document = bsoncxx::builder::basic::document{};
            document.append(bsoncxx::builder::basic::kvp("frequency", frequency));
            document.append(bsoncxx::builder::basic::kvp("name", name.toStdString()));
            document.append(bsoncxx::builder::basic::kvp("hostAddress", hostAddress.toStdString()));
            document.append(bsoncxx::builder::basic::kvp("port", port));
            document.append(bsoncxx::builder::basic::kvp("isPasswordProtected", isPasswordProtected));
            collection.insert_one(document.view());
        }
        catch (const std::exception& e) {
            qDebug() << "MongoDB error:" << e.what();
            return false;
        }

        WavelengthInfo info;
        info.frequency = frequency;
        info.name = name.isEmpty() ? QString("Wavelength-%1").arg(frequency) : name;
        info.isPasswordProtected = isPasswordProtected;
        info.password = password;
        info.hostId = QUuid::createUuid().toString();
        info.isHost = true;
        info.hostAddress = hostAddress;
        info.hostPort = port;

        qDebug() << "Creating new wavelength server on frequency" << frequency
                 << (isPasswordProtected ? "(password protected)" : "");

        m_wavelengths[frequency] = info;
        m_activeWavelength = frequency;

        connect(m_tcpServer, &QTcpServer::newConnection, this, &WavelengthManager::handleNewConnection);

        emit wavelengthCreated(frequency);
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
            std::string hostAddressStr = static_cast<std::string>(doc["hostAddress"].get_string().value);
            int hostPort = doc["port"].get_int32().value;
            std::string nameStr = static_cast<std::string>(doc["name"].get_string().value);

            if (isPasswordProtected && password.isEmpty()) {
                qDebug() << "Password is required for this wavelength";
                return false;
            }

            QTcpSocket* socket = new QTcpSocket(this);
            socket->connectToHost(QString::fromStdString(hostAddressStr), hostPort);

            if (!socket->waitForConnected(5000)) {
                qDebug() << "Failed to connect to host:" << socket->errorString();
                delete socket;
                return false;
            }

            QJsonObject authData;
            authData["type"] = "auth";
            authData["password"] = password;
            authData["clientId"] = QUuid::createUuid().toString();

            QJsonDocument doc_json(authData);
            socket->write(doc_json.toJson(QJsonDocument::Compact));

            WavelengthInfo info;
            info.frequency = frequency;
            info.name = QString::fromStdString(nameStr);
            info.isPasswordProtected = isPasswordProtected;
            info.password = password;
            info.isHost = false;
            info.hostAddress = QString::fromStdString(hostAddressStr);
            info.hostPort = hostPort;
            info.socket = socket;

            connect(socket, &QTcpSocket::readyRead, this, &WavelengthManager::handleClientMessage);
            connect(socket, &QTcpSocket::disconnected, this, &WavelengthManager::handleServerDisconnect);

            m_wavelengths[frequency] = info;
            m_activeWavelength = frequency;

            qDebug() << "Joined wavelength" << frequency;
            emit wavelengthJoined(frequency);
            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "MongoDB error:" << e.what();
            return false;
        }
    }

    void leaveWavelength() {
        if (m_activeWavelength == -1) {
            qDebug() << "Not connected to any wavelength";
            return;
        }

        int freq = m_activeWavelength;

        if (m_wavelengths.contains(freq) && m_wavelengths[freq].isHost) {
            closeWavelength(freq);
        } else if (m_wavelengths.contains(freq)) {
            if (m_wavelengths[freq].socket) {
                QJsonObject disconnectMsg;
                disconnectMsg["type"] = "clientDisconnect";

                QJsonDocument doc(disconnectMsg);
                m_wavelengths[freq].socket->write(doc.toJson(QJsonDocument::Compact));
                m_wavelengths[freq].socket->disconnectFromHost();
                m_wavelengths[freq].socket->deleteLater();
            }
            m_wavelengths.remove(freq);
        }

        m_activeWavelength = -1;

        qDebug() << "Left wavelength" << freq;
        emit wavelengthLeft(freq);
    }

    void closeWavelength(int frequency) {
        if (!m_wavelengths.contains(frequency)) {
            qDebug() << "Wavelength" << frequency << "does not exist";
            return;
        }

        if (!m_wavelengths[frequency].isHost) {
            qDebug() << "Cannot close wavelength" << frequency << "- not the host";
            return;
        }

        qDebug() << "Closing wavelength" << frequency;

        QJsonObject closeMsg;
        closeMsg["type"] = "serverShutdown";
        QJsonDocument doc(closeMsg);
        QByteArray message = doc.toJson(QJsonDocument::Compact);

        for (const auto& client : m_clients) {
            if (client.socket && client.socket->isOpen()) {
                client.socket->write(message);
                client.socket->flush();
            }
        }

        if (m_tcpServer) {
            m_tcpServer->close();
            delete m_tcpServer;
            m_tcpServer = nullptr;
        }

        for (auto& client : m_clients) {
            if (client.socket) {
                client.socket->disconnectFromHost();
                client.socket->deleteLater();
            }
        }
        m_clients.clear();

        try {
            auto collection = m_mongoClient["wavelength"]["active_wavelengths"];
            auto document = bsoncxx::builder::basic::document{};
            document.append(bsoncxx::builder::basic::kvp("frequency", frequency));
            auto filter = document.view();
            collection.delete_one(filter);
        }
        catch (const std::exception& e) {
            qDebug() << "MongoDB error on delete:" << e.what();
        }

        m_wavelengths.remove(frequency);

        if (m_activeWavelength == frequency) {
            m_activeWavelength = -1;
        }

        emit wavelengthClosed(frequency);
    }

    void sendMessage(const QString& message) {
        if (m_activeWavelength == -1) {
            qDebug() << "Not connected to any wavelength";
            return;
        }

        qDebug() << "Sending message on wavelength" << m_activeWavelength << ":" << message;

        const auto& info = m_wavelengths[m_activeWavelength];

        if (info.isHost) {
            QJsonObject msgObj;
            msgObj["type"] = "message";
            msgObj["sender"] = "Host";
            msgObj["content"] = message;

            QJsonDocument doc(msgObj);
            QByteArray messageData = doc.toJson(QJsonDocument::Compact);

            for (const auto& client : m_clients) {
                if (client.socket && client.socket->isOpen()) {
                    client.socket->write(messageData);
                }
            }

            emit messageSent(m_activeWavelength, message);
            emit messageReceived(m_activeWavelength, "Host: " + message);
        } else {
            if (info.socket && info.socket->isOpen()) {
                QJsonObject msgObj;
                msgObj["type"] = "message";
                msgObj["content"] = message;

                QJsonDocument doc(msgObj);
                info.socket->write(doc.toJson(QJsonDocument::Compact));

                emit messageSent(m_activeWavelength, message);
            }
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
        QTcpSocket* clientSocket = m_tcpServer->nextPendingConnection();

        if (!clientSocket) {
            return;
        }

        qDebug() << "New client connected:" << clientSocket->peerAddress().toString();

        ClientInfo newClient;
        newClient.socket = clientSocket;
        newClient.clientId = "pending";
        m_pendingClients.append(newClient);

        connect(clientSocket, &QTcpSocket::readyRead, this, &WavelengthManager::handleServerMessage);
        connect(clientSocket, &QTcpSocket::disconnected, this, &WavelengthManager::handleClientDisconnect);
    }

    void handleServerMessage() {
        QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket) return;

        QByteArray data = socket->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

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
                QByteArray forwardData = forwardDoc.toJson(QJsonDocument::Compact);

                for (const auto& client : m_clients) {
                    if (client.socket && client.socket != socket) {
                        client.socket->write(forwardData);
                    }
                }
            }
        }
    }

    void handleClientMessage() {
        QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket || m_activeWavelength == -1) return;

        QByteArray data = socket->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "Invalid message format received from server";
            return;
        }

        QJsonObject msgObj = doc.object();
        QString msgType = msgObj["type"].toString();

        if (msgType == "message") {
            QString sender = msgObj["sender"].toString();
            QString content = msgObj["content"].toString();

            QString formattedMessage = sender + ": " + content;
            emit messageReceived(m_activeWavelength, formattedMessage);
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

                socket->disconnectFromHost();
                socket->deleteLater();

                if (m_wavelengths.contains(m_activeWavelength)) {
                    m_wavelengths.remove(m_activeWavelength);
                }

                m_activeWavelength = -1;

                emit authenticationFailed(m_activeWavelength);
            }
        }
    }

    void handleClientAuth(QTcpSocket* socket, const QJsonObject& msgObj) {
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
            socket->write(doc.toJson(QJsonDocument::Compact));
            socket->disconnectFromHost();
        } else {
            QJsonDocument doc(response);
            socket->write(doc.toJson(QJsonDocument::Compact));

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
        QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket) return;

        qDebug() << "Client disconnected:" << socket->peerAddress().toString();

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
        QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket) return;

        qDebug() << "Disconnected from server";

        if (m_activeWavelength != -1 && m_wavelengths.contains(m_activeWavelength)) {
            int freq = m_activeWavelength;

            if (m_wavelengths[freq].socket == socket) {
                m_wavelengths.remove(freq);
                m_activeWavelength = -1;

                emit wavelengthClosed(freq);
            }
        }

        socket->deleteLater();
    }

private:
    WavelengthManager(QObject* parent = nullptr) : QObject(parent), m_activeWavelength(-1), m_tcpServer(nullptr) {
        try {
            static mongocxx::instance instance{};
            m_mongoClient = mongocxx::client{mongocxx::uri{"mongodb+srv://wolfiksw:PrincePolo1@testcluster.taymr.mongodb.net/"}};

            auto db = m_mongoClient["wavelength"];
            auto collection = db["active_wavelengths"];

            collection.delete_many({});

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

        if (m_tcpServer) {
            m_tcpServer->close();
            delete m_tcpServer;
        }
    }

    WavelengthManager(const WavelengthManager&) = delete;
    WavelengthManager& operator=(const WavelengthManager&) = delete;

    QMap<int, WavelengthInfo> m_wavelengths;
    int m_activeWavelength;
    QTcpServer* m_tcpServer;
    QList<ClientInfo> m_clients;
    QList<ClientInfo> m_pendingClients;
    mongocxx::client m_mongoClient;
};

#endif // WAVELENGTH_MANAGER_H