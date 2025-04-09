#ifndef WAVELENGTH_JOINER_H
#define WAVELENGTH_JOINER_H

#include <QObject>
#include <QString>

#include "../../../database/database_manager.h"
#include "../../../registry/wavelength_registry.h"
#include "../../../auth/authentication_manager.h"
#include "../../../messages/formatter/message_formatter.h"
#include "../../../messages/handler/message_handler.h"
#include "../../../util/wavelength_config.h"

struct JoinResult {
    bool success;
    QString errorReason;
};

class WavelengthJoiner : public QObject {
    Q_OBJECT

public:
    static WavelengthJoiner* getInstance() {
        static WavelengthJoiner instance;
        return &instance;
    }

    JoinResult joinWavelength(double frequency, const QString& password = QString()) {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    DatabaseManager* dbManager = DatabaseManager::getInstance();

    // Check if we're already in this wavelength
    if (registry->hasWavelength(frequency)) {
        qDebug() << "Already joined wavelength" << frequency;
        registry->setActiveWavelength(frequency);
        emit wavelengthJoined(frequency);
        return {true, QString()};
    }

    // Check if we have a pending registration for this frequency
    if (registry->isPendingRegistration(frequency)) {
        qDebug() << "Wavelength" << frequency << "registration is already pending";
        return {false, "WAVELENGTH UNAVAILABLE"};
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
        return {false, "WAVELENGTH UNAVAILABLE"};
    }

        if (isPasswordProtected && password.isEmpty()) {
            registry->removePendingRegistration(frequency);
            emit connectionError("Ta częstotliwość wymaga hasła");
            return {false, "WAVELENGTH PASSWORD PROTECTED"};
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
    connect(socket, &QWebSocket::textMessageReceived, this, [this, frequency, clientId, name, isPasswordProtected, socket, registry](const QString& message) {
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

            if (!success) {
        if (errorMessage == "Password required") {
            emit connectionError("Ta częstotliwość wymaga hasła");
            qDebug() << "Password required for frequency" << frequency;
        } else if (errorMessage == "Invalid password") {
            emit connectionError("Nieprawidłowe hasło");
            qDebug() << "Invalid password for frequency" << frequency;
        } else {
            emit connectionError("Częstotliwość jest niedostępna");
            qDebug() << "Frequency" << frequency << "is unavailable";
        }
        registry->removePendingRegistration(frequency);
        socket->close();
        return;
    }

    if (success) {
        // Join successful
        registry->removePendingRegistration(frequency);

        WavelengthInfo info;
        info.frequency = frequency;
        info.isPasswordProtected = isPasswordProtected;
        info.hostId = msgObj["hostId"].toString();
        info.isHost = false;
        info.socket = socket;

        // Add wavelength to registry
        registry->addWavelength(frequency, info);
        registry->setActiveWavelength(frequency);

        emit wavelengthJoined(frequency);
    }
} else if (msgType == "message" || msgType == "send_message") {
            qDebug() << "Received chat message in wavelength" << frequency;

            // Używamy MessageFormatter do formatowania wiadomości
            QString formattedMsg = MessageFormatter::formatMessage(msgObj, frequency);

            // Emitujemy sygnał o otrzymaniu wiadomości
            emit messageReceived(frequency, formattedMsg);
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
            double activeFreq = registry->getActiveWavelength();

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

    return {true, QString()};
}

signals:
    void wavelengthJoined(double frequency);
    void wavelengthClosed(double frequency);
    void authenticationFailed(double frequency);
    void connectionError(const QString& errorMessage);
    void messageReceived(double frequency, const QString& formattedMessage);
    void wavelengthLeft(double frequency);

private:
    WavelengthJoiner(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthJoiner() {}

    WavelengthJoiner(const WavelengthJoiner&) = delete;
    WavelengthJoiner& operator=(const WavelengthJoiner&) = delete;
};

#endif // WAVELENGTH_JOINER_H