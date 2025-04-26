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

    if (registry->hasWavelength(frequency)) {
        qDebug() << "Wavelength" << frequency << "already exists locally";
        return false;
    }

    if (registry->isPendingRegistration(frequency)) {
        qDebug() << "Wavelength" << frequency << "registration is already pending";
        return false;
    }

    registry->addPendingRegistration(frequency);
    QString hostId = AuthenticationManager::getInstance()->generateClientId();
    bool* connectedCallbackExecuted = new bool(false);
    QTimer* keepAliveTimer = new QTimer(this);

    qDebug() << "Creating WebSocket connection for wavelength" << frequency;
    QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

    auto registerResultHandler = [this, frequency, socket, keepAliveTimer, registry](const QString& message) {
        qDebug() << "[Creator] RegisterResultHandler received message:" << message;
        bool ok = false;
        QJsonObject msgObj = MessageHandler::getInstance()->parseMessage(message, &ok);
        if (!ok) {
            qDebug() << "[Creator] Failed to parse JSON message in RegisterResultHandler";
            return;
        }

        QString msgType = MessageHandler::getInstance()->getMessageType(msgObj);

        if (msgType == "register_result") {
            disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);

            bool success = msgObj["success"].toBool();
            qDebug() << "[Creator] Register result received:" << (success ? "success" : "failure");

            registry->removePendingRegistration(frequency);

            if (success) {
                WavelengthInfo info = registry->getWavelengthInfo(frequency);
                if (info.frequency.isEmpty()) {
                     qWarning() << "[Creator] WavelengthInfo not found after successful registration for" << frequency;
                     info.frequency = frequency;
                     info.hostId = msgObj["hostId"].toString();
                     info.isHost = true;
                     info.socket = socket;
                     registry->addWavelength(frequency, info);
                } else {
                    info.hostId = msgObj["hostId"].toString();
                    registry->updateWavelength(frequency, info);
                }

                registry->setActiveWavelength(frequency);
                keepAliveTimer->start(WavelengthConfig::getInstance()->getKeepAliveInterval());
                qDebug() << "[Creator] Keep-alive timer started for" << frequency;
                emit wavelengthCreated(frequency);
            } else {
                QString errorMsg = msgObj["error"].toString("Unknown error");
                qDebug() << "[Creator] Failed to register wavelength:" << errorMsg;
                emit connectionError(errorMsg);
                keepAliveTimer->stop();
                socket->close();
            }
        } else {
             qDebug() << "[Creator] Received unexpected message type in RegisterResultHandler:" << msgType;
        }
    };

    auto disconnectHandler = [this, frequency, socket, keepAliveTimer, connectedCallbackExecuted, registry]() {
        qDebug() << "[Creator] WebSocket disconnected for wavelength" << frequency;
        keepAliveTimer->stop();

        if (registry->isPendingRegistration(frequency)) {
            qDebug() << "[Creator] Clearing pending registration for frequency" << frequency;
            registry->removePendingRegistration(frequency);
        }

        if (registry->hasWavelength(frequency)) {
            qDebug() << "[Creator] Removing wavelength" << frequency << "due to socket disconnect";
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

    connect(socket, &QWebSocket::disconnected, this, disconnectHandler);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
        this, [this, socket, frequency, connectedCallbackExecuted, registry](QAbstractSocket::SocketError error) {
            qDebug() << "[Creator] WebSocket error for wavelength" << frequency << ":" << socket->errorString() << "(Code:" << error << ")";

            if (registry->isPendingRegistration(frequency)) {
                registry->removePendingRegistration(frequency);
            }

            QString errorMsg = socket->errorString();
            emit connectionError(errorMsg);
        });

    connect(socket, &QWebSocket::connected, this, [this, socket, frequency, isPasswordProtected,
                                                   password, hostId, keepAliveTimer, connectedCallbackExecuted, registerResultHandler, registry]() {
        if (*connectedCallbackExecuted) {
            qDebug() << "[Creator] Connected callback already executed, ignoring";
            return;
        }
        *connectedCallbackExecuted = true;
        qDebug() << "[Creator] WebSocket connected for wavelength" << frequency;

        WavelengthInfo initialInfo;
        initialInfo.frequency = frequency;
        initialInfo.isPasswordProtected = isPasswordProtected;
        initialInfo.password = password;
        initialInfo.hostId = hostId;
        initialInfo.isHost = true;
        initialInfo.socket = socket;
        registry->addWavelength(frequency, initialInfo);

        qDebug() << "[Creator] Setting socket message handlers for" << frequency;
        WavelengthMessageProcessor::getInstance()->setSocketMessageHandlers(socket, frequency);

        qDebug() << "[Creator] Connecting temporary handler for register_result";
        connect(socket, &QWebSocket::textMessageReceived, this, registerResultHandler);

        connect(keepAliveTimer, &QTimer::timeout, socket, [socket](){
            if (socket->isValid()) {
                socket->ping();
            }
        });

        MessageHandler* msgHandler = MessageHandler::getInstance();
        QJsonObject regRequest = msgHandler->createRegisterRequest(frequency, isPasswordProtected, password, hostId);
        QJsonDocument doc(regRequest);
        socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
        qDebug() << "[Creator] Sent register request for wavelength" << frequency;
    });

    QString address = config->getRelayServerAddress();
    int port = config->getRelayServerPort();
    QUrl url(QString("ws://%1:%2").arg(address).arg(port));
    qDebug() << "[Creator] Opening WebSocket connection to URL:" << url.toString();
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