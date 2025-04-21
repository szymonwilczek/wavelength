#ifndef WAVELENGTH_JOINER_H
#define WAVELENGTH_JOINER_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "../../../database/database_manager.h"
#include "../../../registry/wavelength_registry.h"
#include "../../../auth/authentication_manager.h"
#include "../../../messages/wavelength_message_processor.h"
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

    JoinResult joinWavelength(QString frequency, const QString& password = QString()) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        WavelengthConfig* config = WavelengthConfig::getInstance();

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

    // Generate a client ID
    QString clientId = AuthenticationManager::getInstance()->generateClientId();

    // Zmienna do śledzenia czy callback connected został już wywołany
    bool* connectedCallbackExecuted = new bool(false);
        QTimer* keepAliveTimer = new QTimer(this);

    // Tworzymy socket i przechowujemy wskaźnik do niego
    QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

    // Definiujemy obsługę wiadomości
    connect(socket, &QWebSocket::textMessageReceived, this, [this, frequency, clientId, password, socket, registry, keepAliveTimer](const QString& message) { // Przekaż timer
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
                    if (errorMessage == "Password required" || errorMessage == "Invalid password") {
                         emit connectionError(errorMessage == "Password required" ? "Ta częstotliwość wymaga hasła" : "Nieprawidłowe hasło");
                         qDebug() << (errorMessage == "Password required" ? "Password required" : "Invalid password") << "for frequency" << frequency;
                         emit authenticationFailed(frequency); // Dodaj sygnał o błędzie autentykacji
                    } else {
                        emit connectionError("Częstotliwość jest niedostępna lub wystąpił błąd: " + errorMessage);
                        qDebug() << "Frequency" << frequency << "is unavailable or error occurred:" << errorMessage;
                    }
                    registry->removePendingRegistration(frequency);
                    keepAliveTimer->stop(); // Zatrzymaj timer
                    socket->close();
                    // socket->deleteLater(); // Usunięcie w disconnectHandler
                    return;
                }

                if (success) {
                    // Join successful
                    registry->removePendingRegistration(frequency);

                    WavelengthInfo info;
                    info.frequency = frequency;
                    // info.isPasswordProtected = isPasswordProtected; // Ustaw, jeśli masz tę informację
                    info.isPasswordProtected = !password.isEmpty(); // Przybliżenie - jeśli podano hasło, zakładamy, że jest chronione
                    info.hostId = msgObj["hostId"].toString();
                    info.isHost = false;
                    info.socket = socket; // Przechowaj wskaźnik na socket w info

                    // Add wavelength to registry
                    registry->addWavelength(frequency, info);
                    registry->setActiveWavelength(frequency);

                    // Połącz przetwarzanie wiadomości przychodzących
                    QObject::connect(socket, &QWebSocket::textMessageReceived,
                                   [frequency](const QString& message) {
                        qDebug() << "Processing incoming message for client wavelength" << frequency;
                        WavelengthMessageProcessor::getInstance()->processIncomingMessage(message, frequency);
                    });

                    // Uruchom timer keep-alive po pomyślnym dołączeniu
                    keepAliveTimer->start(WavelengthConfig::getInstance()->getKeepAliveInterval());

                    emit wavelengthJoined(frequency);
                }
            }
        });

    // Obsługa rozłączenia
    connect(socket, &QWebSocket::disconnected, this, [this, frequency, socket, keepAliveTimer, connectedCallbackExecuted]() {
        qDebug() << "Connection to relay server closed for frequency" << frequency;
        keepAliveTimer->stop();

        WavelengthRegistry* registry = WavelengthRegistry::getInstance();

        if (registry->isPendingRegistration(frequency)) {
            registry->removePendingRegistration(frequency);
        }

        if (registry->hasWavelength(frequency)) {
                 qDebug() << "Removing wavelength" << frequency << "due to socket disconnect";
                 QString activeFreq = registry->getActiveWavelength();
                 registry->removeWavelength(frequency);
                 if (activeFreq == frequency) {
                     registry->setActiveWavelength("-1"); // Ustaw na nieaktywny, jeśli to był aktywny
                     emit wavelengthLeft(frequency); // Emituj sygnał opuszczenia
                 } else {
                     emit wavelengthClosed(frequency); // Emituj sygnał zamknięcia (jeśli nie był aktywny)
                 }
             }

        socket->deleteLater();
        keepAliveTimer->deleteLater(); // Usuń timer
        delete connectedCallbackExecuted; // Usuń wskaźnik
    });

    // Obsługa błędów
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                this, [this, socket, frequency, keepAliveTimer, connectedCallbackExecuted](QAbstractSocket::SocketError error) { // Przekaż timer
                    qDebug() << "WebSocket error:" << error << "-" << socket->errorString();
                    keepAliveTimer->stop(); // Zatrzymaj timer

                    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
                    if (registry->isPendingRegistration(frequency)) {
                        registry->removePendingRegistration(frequency);
                    }

                    emit connectionError("Błąd połączenia: " + socket->errorString());

                    // Nie usuwaj socket/timer tutaj, disconnectHandler się tym zajmie
                    // delete connectedCallbackExecuted; // Usunięcie w disconnectHandler
                });

    // Obsługa połączenia
        connect(socket, &QWebSocket::connected, this, [this, socket, frequency, password, clientId, keepAliveTimer, connectedCallbackExecuted]() { // Przekaż timer
                qDebug() << "Connected to relay server for joining frequency" << frequency;

                // Upewnij się, że callback wywoła się tylko raz
                if (*connectedCallbackExecuted) {
                    qDebug() << "Connected callback already executed, ignoring";
                    return;
                }
                *connectedCallbackExecuted = true;

                // Skonfiguruj timer keep-alive (ale jeszcze go nie uruchamiaj)
                 connect(keepAliveTimer, &QTimer::timeout, socket, [socket](){
                     if (socket->isValid()) {
                         qDebug() << "Sending ping...";
                         socket->ping();
                     }
                 });

                // Tworzymy obiekt dołączenia
                QJsonObject joinData;
                joinData["type"] = "join_wavelength";
                joinData["frequency"] = frequency;
                if (!password.isEmpty()) { // Wysyłaj hasło tylko jeśli nie jest puste
                     joinData["password"] = password;
                }
                joinData["clientId"] = clientId;

                // Wysyłamy żądanie dołączenia
                QJsonDocument doc(joinData);
                QString message = doc.toJson(QJsonDocument::Compact);
                qDebug() << "Sending join message:" << message;

                socket->sendTextMessage(message);

                // Nie usuwaj connectedCallbackExecuted tutaj, disconnectHandler się tym zajmie
                // delete connectedCallbackExecuted;
            });

    // Łączymy z serwerem
        QString address = config->getRelayServerAddress();
        int port = config->getRelayServerPort();
        QUrl url(QString("ws://%1:%2").arg(address).arg(port));
        qDebug() << "Opening WebSocket connection to URL for joining:" << url.toString();
        socket->open(url);

    return {true, QString()};
}

signals:
    void wavelengthJoined(QString frequency);
    void wavelengthClosed(QString frequency);
    void authenticationFailed(QString frequency);
    void connectionError(const QString& errorMessage);
    void messageReceived(QString frequency, const QString& formattedMessage);
    void wavelengthLeft(QString frequency);

private:
    WavelengthJoiner(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthJoiner() {}

    WavelengthJoiner(const WavelengthJoiner&) = delete;
    WavelengthJoiner& operator=(const WavelengthJoiner&) = delete;
};

#endif // WAVELENGTH_JOINER_H