#ifndef WAVELENGTH_MANAGER_H
#define WAVELENGTH_MANAGER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QList>
#include <QUuid>
#include <QDebug>

#include "database/database_manager.h"
#include "connection/connection_manager.h"
#include "messages/message_handler.h"
#include "auth/authentication_manager.h"
#include "registry/wavelength_registry.h"
#include "network/network_utilities.h"

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

        return DatabaseManager::getInstance()->isFrequencyAvailable(frequency);
    }

    QString getLocalIpAddress() {
        return NetworkUtilities::getInstance()->getLocalIpAddress();
    }

    bool createWavelength(int frequency, const QString& name, bool isPasswordProtected,
                  const QString& password) {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();

    // Check if the wavelength already exists locally
    if (registry->hasWavelength(frequency)) {
        qDebug() << "Wavelength" << frequency << "already exists locally";
        return false;
    }

    // Check if we have a pending registration for this frequency
    if (registry->isPendingRegistration(frequency)) {
        qDebug() << "Wavelength" << frequency << "registration is already pending";
        return false;
    }

    // Check if the frequency is available in the database
    if (!isFrequencyAvailable(frequency)) {
        qDebug() << "Frequency" << frequency << "is not available";
        return false;
    }

    // Mark this frequency as pending registration
    registry->addPendingRegistration(frequency);

    // Generate a client ID for this host
    QString hostId = AuthenticationManager::getInstance()->generateClientId();

    // Zmienna do śledzenia czy callback connected został już wywołany
    bool* connectedCallbackExecuted = new bool(false);

    qDebug() << "Creating WebSocket connection for wavelength" << frequency;

    // Tworzymy socket i przechowujemy wskaźnik do niego
    QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

    // Definiujemy obsługę wiadomości - wspólna dla wszystkich callbacków
    auto messageHandler = [this, frequency, hostId, name, isPasswordProtected, password, socket](const QString& message) {
        qDebug() << "Received message from server for frequency" << frequency << ":" << message;

        bool ok;
        QJsonObject msgObj = MessageHandler::getInstance()->parseMessage(message, &ok);
        if (!ok) {
            qDebug() << "Failed to parse message from server";
            return;
        }

        QString msgType = MessageHandler::getInstance()->getMessageType(msgObj);
        qDebug() << "Message type:" << msgType;

        if (msgType == "register_result") {
            bool success = msgObj["success"].toBool();
            QString errorMessage = msgObj["error"].toString();

            qDebug() << "Registration result received: success =" << success;

            WavelengthRegistry* registry = WavelengthRegistry::getInstance();

            if (success) {
                // Registration successful
                registry->removePendingRegistration(frequency);

                WavelengthInfo info;
                info.frequency = frequency;
                info.name = name;
                info.isPasswordProtected = isPasswordProtected;
                info.password = password;
                info.hostId = hostId;
                info.isHost = true;
                info.socket = socket; // Bezpośrednie przypisanie socketa

                // Register the password if needed
                if (info.isPasswordProtected) {
                    AuthenticationManager::getInstance()->registerPassword(frequency, password);
                }

                // Add wavelength to registry
                qDebug() << "Adding wavelength" << frequency << "to registry";
                registry->addWavelength(frequency, info);
                registry->setActiveWavelength(frequency);

                // Add to database
                DatabaseManager::getInstance()->addWavelength(
                    frequency, info.name, info.isPasswordProtected, hostId);

                qDebug() << "Emitting wavelengthCreated signal for frequency" << frequency;
                emit wavelengthCreated(frequency);
            } else {
                // Registration failed
                qDebug() << "Registration failed:" << errorMessage;
                registry->removePendingRegistration(frequency);
                emit connectionError(errorMessage.isEmpty() ? "Failed to register wavelength" : errorMessage);

                // Disconnect socket
                socket->close();
            }
        } if (msgType == "message" || msgType == "send_message") {
    qDebug() << "\n===== PEŁNA WIADOMOŚĆ =====";
    qDebug() << "Typ:" << msgType;
    qDebug() << "Oryginalna treść JSON:" << message;
    qDebug() << "Parsowane pola:";
    for (auto it = msgObj.constBegin(); it != msgObj.constEnd(); ++it) {
        qDebug() << " - " << it.key() << ":" << it.value().toString();
    }
    qDebug() << "==========================\n";

    QString messageId = msgObj["messageId"].toString();
    int msgFrequency = msgObj["frequency"].toInt();

    // Check if we've seen this message before
    if (!MessageHandler::getInstance()->isMessageProcessed(messageId)) {
        MessageHandler::getInstance()->markMessageAsProcessed(messageId);

        // Sprawdź czy to wiadomość dla tej częstotliwości
        if (msgFrequency == frequency) {
            QString content;
            QString senderName;

            // Ustal treść wiadomości
            if (msgObj.contains("content")) {
                content = msgObj["content"].toString();
            }

            // Jeśli treść jest pusta, a to jest wiadomość od nas, sprawdźmy w cache'u
            if (content.isEmpty() && msgObj.contains("isSelf") && msgObj["isSelf"].toBool()) {
                if (m_sentMessages.contains(messageId)) {
                    content = m_sentMessages[messageId];
                    m_sentMessages.remove(messageId);
                }
            }

            // Jeśli treść jest nadal pusta, próbujmy ostatniej szansy
            if (content.isEmpty()) {
                // Sprawdź, czy w oryginalnym tekście JSON jest content w ramach innego pola
                // lub struktury zagnieżdżonej
                QString jsonStr = message.toLower();
                int contentPos = jsonStr.indexOf("content");
                if (contentPos >= 0) {
                    int valueStart = jsonStr.indexOf(":", contentPos);
                    if (valueStart > 0) {
                        valueStart++; // Przejdź za dwukropek

                        // Przejdź za spacje
                        while (valueStart < jsonStr.length() && jsonStr[valueStart].isSpace()) {
                            valueStart++;
                        }

                        // Sprawdź czy to string (zaczyna się od ")
                        if (valueStart < jsonStr.length() && jsonStr[valueStart] == '"') {
                            valueStart++; // Przejdź za cudzysłów
                            int valueEnd = jsonStr.indexOf("\"", valueStart);
                            if (valueEnd > valueStart) {
                                content = message.mid(valueStart, valueEnd - valueStart);
                            }
                        }
                    }
                }
            }

            // Ustal nazwę nadawcy
            if (msgObj.contains("sender")) {
                senderName = msgObj["sender"].toString();
            } else if (msgObj.contains("senderId")) {
                QString senderId = msgObj["senderId"].toString();
                WavelengthInfo info = WavelengthRegistry::getInstance()->getWavelengthInfo(frequency);

                if (senderId == info.hostId) {
                    senderName = "Host";
                } else {
                    senderName = "User_" + senderId.left(5);
                }
            }

            // Ostatnia próba - jeśli nadal nie mamy treści lub nadawcy
            if (content.isEmpty()) {
                content = "[treść nieodczytana]";
            }

            if (senderName.isEmpty()) {
                senderName = "Unknown";
            }

            // Format wiadomości
            QString timestamp = QTime::currentTime().toString("[hh:mm:ss]");
            QString formattedMessage = QString("%1 %2: %3").arg(timestamp).arg(senderName).arg(content);

            qDebug() << "Sformatowana wiadomość:" << formattedMessage;
            emit messageReceived(msgFrequency, formattedMessage);
        }
    }
}
    };

    // Definiujemy obsługę rozłączenia
    auto disconnectHandler = [this, frequency, socket]() {
        qDebug() << "Connection to relay server closed for frequency" << frequency;

        WavelengthRegistry* registry = WavelengthRegistry::getInstance();

        if (registry->isPendingRegistration(frequency)) {
            registry->removePendingRegistration(frequency);
        }

        if (registry->hasWavelength(frequency)) {
            int activeFreq = registry->getActiveWavelength();

            registry->removeWavelength(frequency);

            if (activeFreq == frequency) {
                emit wavelengthLeft(frequency);
            }
        }

        // Socket zostanie usunięty automatycznie przez Qt
    };

    // Podłączamy obsługę wiadomości i rozłączenia przed połączeniem
    connect(socket, &QWebSocket::textMessageReceived, this, messageHandler);
    connect(socket, &QWebSocket::disconnected, this, disconnectHandler);

    // Podłącz obsługę błędów
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
        this, [this, socket, frequency, connectedCallbackExecuted](QAbstractSocket::SocketError error) {
            qDebug() << "WebSocket error:" << error << "-" << socket->errorString();

            WavelengthRegistry* registry = WavelengthRegistry::getInstance();
            if (registry->isPendingRegistration(frequency)) {
                registry->removePendingRegistration(frequency);
            }

            emit connectionError("Connection error: " + socket->errorString());

            // Zapobieganie wyciekowi pamięci
            delete connectedCallbackExecuted;
        });

    // Definiujemy obsługę połączenia
    connect(socket, &QWebSocket::connected, this, [this, socket, frequency, name, isPasswordProtected,
                                                   password, hostId, connectedCallbackExecuted]() {
        qDebug() << "Connected to relay server for frequency" << frequency;

        // Upewnij się, że callback wywoła się tylko raz
        if (*connectedCallbackExecuted) {
            qDebug() << "Connected callback already executed, ignoring";
            return;
        }
        *connectedCallbackExecuted = true;

        // Tworzymy obiekt rejestracyjny
        QJsonObject regData;
        regData["type"] = "register_wavelength";
        regData["frequency"] = frequency;
        regData["name"] = name;
        regData["isPasswordProtected"] = isPasswordProtected;
        regData["password"] = password;
        regData["hostId"] = hostId;

        // Wysyłamy żądanie rejestracji bezpośrednio na socket
        QJsonDocument doc(regData);
        QString message = doc.toJson(QJsonDocument::Compact);
        qDebug() << "Sending registration message:" << message;

        // Bezpośrednie użycie socketa zamiast pobierania z rejestru
        socket->sendTextMessage(message);

        // Zapobieganie wyciekowi pamięci - usuwamy po zakończeniu
        delete connectedCallbackExecuted;
    });

    // Łączymy z serwerem
    QString url = QString("ws://%1:%2").arg(RELAY_SERVER_ADDRESS.mid(5)).arg(RELAY_SERVER_PORT);
    qDebug() << "Opening WebSocket connection to URL:" << url;
    socket->open(QUrl(url));

    return true;
}

    bool joinWavelength(int frequency, const QString& password = QString()) {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    DatabaseManager* dbManager = DatabaseManager::getInstance();

    // Check if we're already in this wavelength
    if (registry->hasWavelength(frequency)) {
        qDebug() << "Already joined wavelength" << frequency;
        registry->setActiveWavelength(frequency);
        emit wavelengthJoined(frequency);
        return true;
    }

    // Check if we have a pending registration for this frequency
    if (registry->isPendingRegistration(frequency)) {
        qDebug() << "Wavelength" << frequency << "registration is already pending";
        return false;
    }

    // Mark as pending
    registry->addPendingRegistration(frequency);

    // Get wavelength details from database
    QString name;
    bool isPasswordProtected;
    if (!dbManager->getWavelengthDetails(frequency, name, isPasswordProtected)) {
        registry->removePendingRegistration(frequency);
        qDebug() << "Failed to get wavelength details from database";
        emit connectionError("Częstotliwość nie istnieje lub nie jest dostępna");
        return false;
    }

    qDebug() << "Joining existing wavelength:" << frequency << "Name:" << name
             << "Password protected:" << isPasswordProtected;

    // Generate a client ID
    QString clientId = AuthenticationManager::getInstance()->generateClientId();

    // Zmienna do śledzenia czy callback connected został już wywołany
    bool* connectedCallbackExecuted = new bool(false);

    // Tworzymy socket i przechowujemy wskaźnik do niego
    QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

    // Definiujemy obsługę wiadomości
    connect(socket, &QWebSocket::textMessageReceived, this, [this, frequency, clientId, name, isPasswordProtected, socket](const QString& message) {
        qDebug() << "Received message from server for frequency" << frequency << ":" << message;

        bool ok;
        QJsonObject msgObj = MessageHandler::getInstance()->parseMessage(message, &ok);
        if (!ok) {
            qDebug() << "Failed to parse message from server";
            return;
        }

        QString msgType = MessageHandler::getInstance()->getMessageType(msgObj);
        qDebug() << "Message type:" << msgType;

        if (msgType == "join_result") {
            bool success = msgObj["success"].toBool();
            QString errorMessage = msgObj["error"].toString();

            qDebug() << "Join result received: success =" << success;

            WavelengthRegistry* registry = WavelengthRegistry::getInstance();

            if (success) {
                // Join successful
                registry->removePendingRegistration(frequency);

                WavelengthInfo info;
                info.frequency = frequency;
                info.name = name;
                info.isPasswordProtected = isPasswordProtected;
                info.hostId = msgObj["hostId"].toString();
                info.isHost = false;
                info.socket = socket; // Bezpośrednie przypisanie socketa

                // Add wavelength to registry
                qDebug() << "Adding joined wavelength" << frequency << "to registry";
                registry->addWavelength(frequency, info);
                registry->setActiveWavelength(frequency);

                qDebug() << "Emitting wavelengthJoined signal for frequency" << frequency;
                emit wavelengthJoined(frequency);
            } else {
                // Join failed
                qDebug() << "Join failed:" << errorMessage;
                registry->removePendingRegistration(frequency);
                emit connectionError(errorMessage.isEmpty() ? "Nie udało się dołączyć do pokoju" : errorMessage);

                // Disconnect socket
                socket->close();
            }
        } else if (msgType == "message" || msgType == "send_message") {
    qDebug() << "\n===== PEŁNA WIADOMOŚĆ =====";
    qDebug() << "Typ:" << msgType;
    qDebug() << "Oryginalna treść JSON:" << message;
    qDebug() << "Parsowane pola:";
    for (auto it = msgObj.constBegin(); it != msgObj.constEnd(); ++it) {
        qDebug() << " - " << it.key() << ":" << it.value().toString();
    }
    qDebug() << "==========================\n";

    QString messageId = msgObj["messageId"].toString();
    int msgFrequency = msgObj["frequency"].toInt();

    // Check if we've seen this message before
    if (!MessageHandler::getInstance()->isMessageProcessed(messageId)) {
        MessageHandler::getInstance()->markMessageAsProcessed(messageId);

        // Sprawdź czy to wiadomość dla tej częstotliwości
        if (msgFrequency == frequency) {
            QString content;
            QString senderName;

            // Ustal treść wiadomości
            if (msgObj.contains("content")) {
                content = msgObj["content"].toString();
            }

            // Jeśli treść jest pusta, a to jest wiadomość od nas, sprawdźmy w cache'u
            if (content.isEmpty() && msgObj.contains("isSelf") && msgObj["isSelf"].toBool()) {
                if (m_sentMessages.contains(messageId)) {
                    content = m_sentMessages[messageId];
                    m_sentMessages.remove(messageId);
                }
            }

            // Jeśli treść jest nadal pusta, próbujmy ostatniej szansy
            if (content.isEmpty()) {
                // Sprawdź, czy w oryginalnym tekście JSON jest content w ramach innego pola
                // lub struktury zagnieżdżonej
                QString jsonStr = message.toLower();
                int contentPos = jsonStr.indexOf("content");
                if (contentPos >= 0) {
                    int valueStart = jsonStr.indexOf(":", contentPos);
                    if (valueStart > 0) {
                        valueStart++; // Przejdź za dwukropek

                        // Przejdź za spacje
                        while (valueStart < jsonStr.length() && jsonStr[valueStart].isSpace()) {
                            valueStart++;
                        }

                        // Sprawdź czy to string (zaczyna się od ")
                        if (valueStart < jsonStr.length() && jsonStr[valueStart] == '"') {
                            valueStart++; // Przejdź za cudzysłów
                            int valueEnd = jsonStr.indexOf("\"", valueStart);
                            if (valueEnd > valueStart) {
                                content = message.mid(valueStart, valueEnd - valueStart);
                            }
                        }
                    }
                }
            }

            // Ustal nazwę nadawcy
            if (msgObj.contains("sender")) {
                senderName = msgObj["sender"].toString();
            } else if (msgObj.contains("senderId")) {
                QString senderId = msgObj["senderId"].toString();
                WavelengthInfo info = WavelengthRegistry::getInstance()->getWavelengthInfo(frequency);

                if (senderId == info.hostId) {
                    senderName = "Host";
                } else {
                    senderName = "User_" + senderId.left(5);
                }
            }

            // Ostatnia próba - jeśli nadal nie mamy treści lub nadawcy
            if (content.isEmpty()) {
                content = "[treść nieodczytana]";
            }

            if (senderName.isEmpty()) {
                senderName = "Unknown";
            }

            // Format wiadomości
            QString timestamp = QTime::currentTime().toString("[hh:mm:ss]");
            QString formattedMessage = QString("%1 %2: %3").arg(timestamp).arg(senderName).arg(content);

            qDebug() << "Sformatowana wiadomość:" << formattedMessage;
            emit messageReceived(msgFrequency, formattedMessage);
        }
    }
}

    });

    // Obsługa rozłączenia
    connect(socket, &QWebSocket::disconnected, this, [this, frequency, socket]() {
        qDebug() << "Connection to relay server closed for frequency" << frequency;

        WavelengthRegistry* registry = WavelengthRegistry::getInstance();

        if (registry->isPendingRegistration(frequency)) {
            registry->removePendingRegistration(frequency);
        }

        if (registry->hasWavelength(frequency)) {
            int activeFreq = registry->getActiveWavelength();

            registry->removeWavelength(frequency);

            if (activeFreq == frequency) {
                emit wavelengthLeft(frequency);
            }
        }
    });

    // Obsługa błędów
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
        this, [this, socket, frequency, connectedCallbackExecuted](QAbstractSocket::SocketError error) {
            qDebug() << "WebSocket error:" << error << "-" << socket->errorString();

            WavelengthRegistry* registry = WavelengthRegistry::getInstance();
            if (registry->isPendingRegistration(frequency)) {
                registry->removePendingRegistration(frequency);
            }

            emit connectionError("Błąd połączenia: " + socket->errorString());

            // Zapobieganie wyciekowi pamięci
            delete connectedCallbackExecuted;
        });

    // Obsługa połączenia
    connect(socket, &QWebSocket::connected, this, [this, socket, frequency, password, clientId, connectedCallbackExecuted]() {
        qDebug() << "Connected to relay server for joining frequency" << frequency;

        // Upewnij się, że callback wywoła się tylko raz
        if (*connectedCallbackExecuted) {
            qDebug() << "Connected callback already executed, ignoring";
            return;
        }
        *connectedCallbackExecuted = true;

        // Tworzymy obiekt dołączenia
        QJsonObject joinData;
        joinData["type"] = "join_wavelength";
        joinData["frequency"] = frequency;
        joinData["password"] = password;
        joinData["clientId"] = clientId;

        // Wysyłamy żądanie dołączenia
        QJsonDocument doc(joinData);
        QString message = doc.toJson(QJsonDocument::Compact);
        qDebug() << "Sending join message:" << message;

        socket->sendTextMessage(message);

        // Zapobieganie wyciekowi pamięci
        delete connectedCallbackExecuted;
    });

    // Łączymy z serwerem
    QString url = QString("ws://%1:%2").arg(RELAY_SERVER_ADDRESS.mid(5)).arg(RELAY_SERVER_PORT);
    qDebug() << "Opening WebSocket connection to URL for joining:" << url;
    socket->open(QUrl(url));

    return true;
}

    void leaveWavelength() {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        int freq = registry->getActiveWavelength();

        if (freq == -1) {
            qDebug() << "No active wavelength to leave";
            return;
        }

        if (!registry->hasWavelength(freq)) {
            qDebug() << "Active wavelength" << freq << "not found in registry";
            return;
        }

        WavelengthInfo info = registry->getWavelengthInfo(freq);

        if (info.socket) {
            if (info.socket->isValid()) {
                if (info.isHost) {
                    // If we're host, close the wavelength
                    closeWavelength(freq);
                } else {
                    // Otherwise just leave it
                    QJsonObject leaveData = MessageHandler::getInstance()->createLeaveRequest(freq, false);
                    QJsonDocument doc(leaveData);
                    info.socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

                    // Close socket
                    info.socket->close();

                    // Remove from registry
                    registry->removeWavelength(freq);

                    emit wavelengthLeft(freq);
                }
            }
        }
    }

    bool getWavelengthInfo(int frequency, bool* isHost = nullptr) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();

        if (!registry->hasWavelength(frequency)) {
            return false;
        }

        WavelengthInfo info = registry->getWavelengthInfo(frequency);

        if (isHost) {
            *isHost = info.isHost;
        }

        return true;
    }

    void closeWavelength(int frequency) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();

        if (!registry->hasWavelength(frequency)) {
            qDebug() << "Cannot close - wavelength" << frequency << "not found";
            return;
        }

        WavelengthInfo info = registry->getWavelengthInfo(frequency);

        if (!info.isHost) {
            qDebug() << "Cannot close wavelength" << frequency << "- not the host";
            return;
        }

        // Mark as closing to prevent recursive closing
        registry->markWavelengthClosing(frequency);

        // Check if this was the active wavelength
        bool wasActive = (registry->getActiveWavelength() == frequency);

        // Notify relay server
        QPointer<QWebSocket> socketToClose = info.socket;

        if (socketToClose && socketToClose->isValid()) {
            // Send close request
            QJsonObject closeData = MessageHandler::getInstance()->createLeaveRequest(frequency, true);
            QJsonDocument doc(closeData);
            socketToClose->sendTextMessage(doc.toJson(QJsonDocument::Compact));

            // Wait a moment to ensure message is sent before closing
            QTimer::singleShot(100, [socketToClose]() {
                if (socketToClose) {
                    socketToClose->close();
                }
            });
        } else if (socketToClose) {
            socketToClose->deleteLater();
        }

        // Remove from database
        DatabaseManager::getInstance()->removeWavelength(frequency);

        // Clear password
        if (info.isPasswordProtected) {
            AuthenticationManager::getInstance()->removePassword(frequency);
        }

        // Remove from registry
        registry->removeWavelength(frequency);

        if (wasActive) {
            emit wavelengthClosed(frequency);
        }
    }

    bool sendMessage(const QString& message) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        int freq = registry->getActiveWavelength();

        if (freq == -1) {
            qDebug() << "Cannot send message - no active wavelength";
            return false;
        }

        if (!registry->hasWavelength(freq)) {
            qDebug() << "Cannot send message - active wavelength not found";
            return false;
        }

        WavelengthInfo info = registry->getWavelengthInfo(freq);
        QWebSocket* socket = info.socket;

        if (!socket) {
            qDebug() << "Cannot send message - no socket for wavelength" << freq;
            return false;
        }

        if (!socket->isValid()) {
            qDebug() << "Cannot send message - socket for wavelength" << freq << "is invalid";
            return false;
        }

        // Generate a sender ID (use host ID if we're host, or client ID otherwise)
        QString senderId = info.isHost ? info.hostId :
                          AuthenticationManager::getInstance()->generateClientId();

        QString messageId = MessageHandler::getInstance()->generateMessageId();

        // Zapamiętujemy treść wiadomości do późniejszego użycia
        m_sentMessages[messageId] = message;

        // Ograniczamy rozmiar cache'u wiadomości
        if (m_sentMessages.size() > 100) {
            auto it = m_sentMessages.begin();
            m_sentMessages.erase(it);
        }

        // Tworzmy obiekt wiadomości
        QJsonObject messageObj;
        messageObj["type"] = "send_message"; // Używamy typu zgodnego z serwerem
        messageObj["frequency"] = freq;
        messageObj["content"] = message;
        messageObj["senderId"] = senderId;
        messageObj["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        messageObj["messageId"] = messageId;

        // Wysyłamy wiadomość
        QJsonDocument doc(messageObj);
        QString jsonMessage = doc.toJson(QJsonDocument::Compact);
        qDebug() << "Sending message to server:" << jsonMessage;
        socket->sendTextMessage(jsonMessage);

        // WAŻNE: NIE emitujemy tutaj localnie sygnału messageReceived!
        // Zamiast tego czekamy na powrót wiadomości z serwera

        return true;
    }

    int getActiveWavelength() const {
        return WavelengthRegistry::getInstance()->getActiveWavelength();
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

private:
    WavelengthManager(QObject* parent = nullptr) : QObject(parent) {
        // Periodically clean up expired sessions
        QTimer* cleanupTimer = new QTimer(this);
        connect(cleanupTimer, &QTimer::timeout, this, []() {
            AuthenticationManager::getInstance()->cleanupExpiredSessions();
        });
        cleanupTimer->start(3600000); // run every hour
    }

    ~WavelengthManager() {}

    WavelengthManager(const WavelengthManager&) = delete;
    WavelengthManager& operator=(const WavelengthManager&) = delete;
    QMap<QString, QString> m_sentMessages;
};

#endif // WAVELENGTH_MANAGER_H