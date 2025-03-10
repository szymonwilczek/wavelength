#ifndef WAVELENGTH_JOINER_H
#define WAVELENGTH_JOINER_H

#include <QObject>
#include <QString>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QDebug>

#include "../database/database_manager.h"
#include "../registry/wavelength_registry.h"
#include "../auth/authentication_manager.h"
#include "../messages/message_handler.h"
#include "../util/wavelength_config.h"

class WavelengthJoiner : public QObject {
    Q_OBJECT

public:
    static WavelengthJoiner* getInstance() {
        static WavelengthJoiner instance;
        return &instance;
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
                if (msgObj.contains(messageId)) {
                    content = msgObj[messageId].toString();
                    msgObj.remove(messageId);
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
            emit messageReceived(formattedMessage);
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
        const QString RELAY_SERVER_ADDRESS = "ws://localhost";
        const int RELAY_SERVER_PORT = 3000;
    QString url = QString("ws://%1:%2").arg(RELAY_SERVER_ADDRESS.mid(5)).arg(RELAY_SERVER_PORT);
    qDebug() << "Opening WebSocket connection to URL for joining:" << url;
    socket->open(QUrl(url));

    return true;
}

signals:
    void wavelengthJoined(int frequency);
    void wavelengthClosed(int frequency);
    void authenticationFailed(int frequency);
    void connectionError(const QString& errorMessage);
    void messageReceived(const QString& formattedMessage);
    void wavelengthLeft(int frequency);

private:
    WavelengthJoiner(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthJoiner() {}

    WavelengthJoiner(const WavelengthJoiner&) = delete;
    WavelengthJoiner& operator=(const WavelengthJoiner&) = delete;
};

#endif // WAVELENGTH_JOINER_H