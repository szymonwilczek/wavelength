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
        auto messageHandler = [this, frequency, clientId, password, socket](const QString& message) {
            qDebug() << "WebSocket received message for join wavelength" << frequency << ":" << message;
            bool ok = false;
            QJsonObject msgObj = MessageHandler::getInstance()->parseMessage(message, &ok);
            if (!ok) {
                qDebug() << "Failed to parse JSON message";
                return;
            }

            QString msgType = MessageHandler::getInstance()->getMessageType(msgObj);

            // Handle auth_result message
            if (msgType == "auth_result") {
                bool success = msgObj["success"].toBool();
                qDebug() << "Auth result received:" << (success ? "success" : "failure");

                WavelengthRegistry* registry = WavelengthRegistry::getInstance();
                registry->removePendingRegistration(frequency);

                if (success) {
                    QString name = msgObj["wavelengthName"].toString();
                    QString hostId = msgObj["hostId"].toString();
                    bool isPasswordProtected = msgObj["isPasswordProtected"].toBool();

                    WavelengthInfo info;
                    info.frequency = frequency;
                    info.name = name;
                    info.isPasswordProtected = isPasswordProtected;
                    info.password = password;
                    info.hostId = hostId;
                    info.isHost = false;
                    info.socket = socket;

                    registry->addWavelength(frequency, info);
                    registry->setActiveWavelength(frequency);

                    emit wavelengthJoined(frequency);
                } else {
                    QString errorMsg = msgObj["error"].toString("Authentication failed");
                    qDebug() << "Failed to join wavelength:" << errorMsg;
                    emit authenticationFailed(frequency);
                    emit connectionError(errorMsg);

                    socket->close();
                    socket->deleteLater();
                }
            }
        };

        // Definiujemy obsługę rozłączenia
        auto disconnectHandler = [this, frequency, socket, connectedCallbackExecuted]() {
            qDebug() << "WebSocket disconnected for join wavelength" << frequency;
            WavelengthRegistry* registry = WavelengthRegistry::getInstance();

            if (registry->isPendingRegistration(frequency)) {
                qDebug() << "Clearing pending registration for frequency" << frequency;
                registry->removePendingRegistration(frequency);
            }

            if (registry->hasWavelength(frequency)) {
                qDebug() << "Removing wavelength" << frequency << "due to socket disconnect";
                int activeFreq = registry->getActiveWavelength();
                
                registry->removeWavelength(frequency);
                
                if (activeFreq == frequency) {
                    registry->setActiveWavelength(-1);
                }
                
                emit wavelengthClosed(frequency);
            }
            
            socket->deleteLater();
            delete connectedCallbackExecuted;
        };

        // Podłączamy obsługę wiadomości i rozłączenia przed połączeniem
        connect(socket, &QWebSocket::textMessageReceived, this, messageHandler);
        connect(socket, &QWebSocket::disconnected, this, disconnectHandler);

        // Podłącz obsługę błędów
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this, socket, frequency, connectedCallbackExecuted](QAbstractSocket::SocketError error) {
                qDebug() << "WebSocket error for join wavelength" << frequency << ":" << error;
                
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
        connect(socket, &QWebSocket::connected, this, [this, socket, frequency, password, clientId, 
                                                       connectedCallbackExecuted]() {
            if (*connectedCallbackExecuted) {
                qDebug() << "Connected callback already executed for join, ignoring";
                return;
            }
            
            *connectedCallbackExecuted = true;
            qDebug() << "WebSocket connected for join wavelength" << frequency;
            
            MessageHandler* msgHandler = MessageHandler::getInstance();
            QJsonObject authRequest = msgHandler->createAuthRequest(frequency, password, clientId);
            
            QJsonDocument doc(authRequest);
            socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
            qDebug() << "Sent auth request for wavelength" << frequency;
        });

        // Łączymy z serwerem
        QString url = QString("ws://localhost:3000");
        qDebug() << "Opening WebSocket connection to URL:" << url;
        socket->open(QUrl(url));

        return true;
    }

signals:
    void wavelengthJoined(int frequency);
    void wavelengthClosed(int frequency);
    void authenticationFailed(int frequency);
    void connectionError(const QString& errorMessage);

private:
    WavelengthJoiner(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthJoiner() {}

    WavelengthJoiner(const WavelengthJoiner&) = delete;
    WavelengthJoiner& operator=(const WavelengthJoiner&) = delete;
};

#endif // WAVELENGTH_JOINER_H