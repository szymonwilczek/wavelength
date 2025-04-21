#ifndef WAVELENGTH_CREATOR_H
#define WAVELENGTH_CREATOR_H

#include <QObject>
#include <QString>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QDebug>
#include <QTimer>

#include "../../../database/database_manager.h"
#include "../../../registry/wavelength_registry.h"
#include "../../../auth/authentication_manager.h"
#include "../../../messages/handler/message_handler.h"
#include "../../../messages/wavelength_message_processor.h"
#include "../../../util/wavelength_config.h"

class WavelengthCreator : public QObject {
    Q_OBJECT

public:
    static WavelengthCreator* getInstance() {
        static WavelengthCreator instance;
        return &instance;
    }

    bool createWavelength(QString frequency, bool isPasswordProtected,
                  const QString& password) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        WavelengthConfig* config = WavelengthConfig::getInstance();

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

        // // Check if the frequency is available in the database
        // if (!DatabaseManager::getInstance()->isFrequencyAvailable(frequency)) {
        //     qDebug() << "Frequency" << frequency << "is not available";
        //     return false;
        // }

        // Mark this frequency as pending registration
        registry->addPendingRegistration(frequency);

        // Generate a client ID for this host
        QString hostId = AuthenticationManager::getInstance()->generateClientId();

        // Zmienna do śledzenia czy callback connected został już wywołany
        bool* connectedCallbackExecuted = new bool(false);
        QTimer* keepAliveTimer = new QTimer(this);

        qDebug() << "Creating WebSocket connection for wavelength" << frequency;

        // Tworzymy socket i przechowujemy wskaźnik do niego
        QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

        // Definiujemy obsługę wiadomości - wspólna dla wszystkich callbacków
        auto messageHandler = [this, frequency, hostId, isPasswordProtected, password, socket, keepAliveTimer](const QString& message) { // Przekaż timer
            qDebug() << "WebSocket received message for wavelength" << frequency << ":" << message;
            bool ok = false;
            QJsonObject msgObj = MessageHandler::getInstance()->parseMessage(message, &ok);
            if (!ok) {
                qDebug() << "Failed to parse JSON message";
                return;
            }

            QString msgType = MessageHandler::getInstance()->getMessageType(msgObj);
            QString messageId = MessageHandler::getInstance()->getMessageId(msgObj);

            // Handle register_result message
            if (msgType == "register_result") {
                bool success = msgObj["success"].toBool();
                qDebug() << "Register result received:" << (success ? "success" : "failure");

                WavelengthRegistry* registry = WavelengthRegistry::getInstance();
                registry->removePendingRegistration(frequency);

                if (success) {
                    WavelengthInfo info;
                    info.frequency = frequency;
                    info.isPasswordProtected = isPasswordProtected;
                    info.password = password;
                    info.hostId = hostId;
                    info.isHost = true;
                    info.socket = socket; // Przechowaj wskaźnik na socket w info

                    registry->addWavelength(frequency, info);
                    registry->setActiveWavelength(frequency);

                    // Połącz przetwarzanie wiadomości przychodzących
                    QObject::connect(socket, &QWebSocket::textMessageReceived,
                                   [frequency](const QString& message) {
                        qDebug() << "Processing incoming message for host wavelength" << frequency;
                        WavelengthMessageProcessor::getInstance()->processIncomingMessage(message, frequency);
                    });

                    // Uruchom timer keep-alive po pomyślnej rejestracji
                    keepAliveTimer->start(WavelengthConfig::getInstance()->getKeepAliveInterval());

                    emit wavelengthCreated(frequency);
                } else {
                    QString errorMsg = msgObj["error"].toString("Unknown error");
                    qDebug() << "Failed to register wavelength:" << errorMsg;
                    emit connectionError(errorMsg);

                    keepAliveTimer->stop(); // Zatrzymaj timer w razie błędu
                    socket->close();
                    // socket->deleteLater(); // Usunięcie w disconnectHandler
                }
            }
            // Można dodać obsługę PONG, jeśli serwer je wysyła
            // connect(socket, &QWebSocket::pong, this, [](quint64 elapsedTime, const QByteArray &payload){ ... });
        };

        // Definiujemy obsługę rozłączenia
        auto disconnectHandler = [this, frequency, socket, keepAliveTimer, connectedCallbackExecuted]() {
            qDebug() << "WebSocket disconnected for wavelength" << frequency;
            WavelengthRegistry* registry = WavelengthRegistry::getInstance();

            if (registry->isPendingRegistration(frequency)) {
                qDebug() << "Clearing pending registration for frequency" << frequency;
                registry->removePendingRegistration(frequency);
            }

            if (registry->hasWavelength(frequency)) {
                qDebug() << "Removing wavelength" << frequency << "due to socket disconnect";
                keepAliveTimer->stop();
                QString activeFreq = registry->getActiveWavelength();
                
                registry->removeWavelength(frequency);

                if (activeFreq == frequency) {
                    registry->setActiveWavelength("-1");
                }
                
                emit wavelengthClosed(frequency);
            }
            
            socket->deleteLater();
            keepAliveTimer->deleteLater();
            delete connectedCallbackExecuted;
        };

        // Podłączamy obsługę wiadomości i rozłączenia przed połączeniem
        connect(socket, &QWebSocket::textMessageReceived, this, messageHandler);
        connect(socket, &QWebSocket::disconnected, this, disconnectHandler);

        // Podłącz obsługę błędów
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this, socket, frequency, connectedCallbackExecuted](QAbstractSocket::SocketError error) {
                qDebug() << "WebSocket error for wavelength" << frequency << ":" << error;
                
                WavelengthRegistry* registry = WavelengthRegistry::getInstance();
                if (registry->isPendingRegistration(frequency)) {
                    registry->removePendingRegistration(frequency);
                }
                
                QString errorMsg = socket->errorString();
                emit connectionError(errorMsg);
                
                socket->deleteLater();
                delete connectedCallbackExecuted;
            });

        // Definiujemy obsługę połączenia
        connect(socket, &QWebSocket::connected, this, [this, socket, frequency, isPasswordProtected,
                                                       password, hostId, keepAliveTimer, connectedCallbackExecuted]() { // Przekaż timer
            if (*connectedCallbackExecuted) {
                qDebug() << "Connected callback already executed, ignoring";
                return;
            }

            *connectedCallbackExecuted = true;
            qDebug() << "WebSocket connected for wavelength" << frequency;

            connect(keepAliveTimer, &QTimer::timeout, socket, [socket](){
                if (socket->isValid()) {
                    qDebug() << "Sending ping...";
                    socket->ping();
                }
            });

            MessageHandler* msgHandler = MessageHandler::getInstance();
            QJsonObject regRequest = msgHandler->createRegisterRequest(frequency, isPasswordProtected, password, hostId);

            QJsonDocument doc(regRequest);
            socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
            qDebug() << "Sent register request for wavelength" << frequency;
        });

        // Łączymy z serwerem
        QString address = config->getRelayServerAddress();
        int port = config->getRelayServerPort();
        QUrl url(QString("ws://%1:%2").arg(address).arg(port));
        qDebug() << "Opening WebSocket connection to URL:" << url.toString();
        socket->open(url);

        return true;
    }

signals:
    void wavelengthCreated(QString frequency);
    void wavelengthClosed(QString frequency);
    void connectionError(const QString& errorMessage);

private:
    WavelengthCreator(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthCreator() {}

    WavelengthCreator(const WavelengthCreator&) = delete;
    WavelengthCreator& operator=(const WavelengthCreator&) = delete;
};

#endif // WAVELENGTH_CREATOR_H