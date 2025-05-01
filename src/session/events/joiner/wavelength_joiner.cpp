#include "wavelength_joiner.h"

#include "../../../auth/authentication_manager.h"

JoinResult WavelengthJoiner::joinWavelength(QString frequency, const QString &password) {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    const WavelengthConfig* config = WavelengthConfig::GetInstance();

    if (registry->hasWavelength(frequency)) {
        qDebug() << "Already joined wavelength" << frequency;
        registry->setActiveWavelength(frequency);
        emit wavelengthJoined(frequency);
        return {true, QString()};
    }

    if (registry->isPendingRegistration(frequency)) {
        qDebug() << "Wavelength" << frequency << "registration is already pending";
        return {false, "WAVELENGTH UNAVAILABLE"};
    }

    registry->addPendingRegistration(frequency);
    QString clientId = AuthenticationManager::GetInstance()->GenerateClientId();
    auto connectedCallbackExecuted = new bool(false);
    auto keepAliveTimer = new QTimer(this);

    auto socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

    auto joinResultHandler = [this, frequency, socket, keepAliveTimer, registry](const QString& message) {
        qDebug() << "[Joiner] JoinResultHandler received message:" << message;

        bool ok;
        QJsonObject msgObj = MessageHandler::getInstance()->parseMessage(message, &ok);
        if (!ok) {
            qDebug() << "[Joiner] Failed to parse message in JoinResultHandler";
            return;
        }

        const QString msgType = MessageHandler::getInstance()->getMessageType(msgObj);
        qDebug() << "[Joiner] Message type in JoinResultHandler:" << msgType;

        if (msgType == "join_result") {
            disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);

            const bool success = msgObj["success"].toBool();
            const QString errorMessage = msgObj["error"].toString();

            registry->removePendingRegistration(frequency);

            if (!success) {
                if (errorMessage == "Password required" || errorMessage == "Invalid password") {
                    emit connectionError(errorMessage == "Password required" ? "Ta częstotliwość wymaga hasła" : "Nieprawidłowe hasło");
                    qDebug() << "[Joiner]" << (errorMessage == "Password required" ? "Password required" : "Invalid password") << "for frequency" << frequency;
                    emit authenticationFailed(frequency);
                } else {
                    emit connectionError("Częstotliwość jest niedostępna lub wystąpił błąd: " + errorMessage);
                    qDebug() << "[Joiner] Frequency" << frequency << "is unavailable or error occurred:" << errorMessage;
                }
                keepAliveTimer->stop();
                socket->close();
                return;
            }

            if (success) {
                qDebug() << "[Joiner] Join successful for frequency" << frequency;
                WavelengthInfo info = registry->getWavelengthInfo(frequency);
                if (info.frequency.isEmpty()) {
                    qWarning() << "[Joiner] WavelengthInfo not found after successful join for" << frequency;
                    info.frequency = frequency;
                    info.hostId = msgObj["hostId"].toString();
                    info.isHost = false;
                    info.socket = socket;
                    registry->addWavelength(frequency, info);
                } else {
                    info.hostId = msgObj["hostId"].toString();
                    registry->updateWavelength(frequency, info);
                }

                registry->setActiveWavelength(frequency);
                keepAliveTimer->start(WavelengthConfig::GetInstance()->GetKeepAliveInterval());
                qDebug() << "[Joiner] Keep-alive timer started for" << frequency;
                emit wavelengthJoined(frequency);
            }
        } else {
            qDebug() << "[Joiner] Received unexpected message type in JoinResultHandler:" << msgType;
        }
    };

    auto disconnectHandler = [this, frequency, socket, keepAliveTimer, connectedCallbackExecuted, registry]() {
        qDebug() << "[Joiner] Connection to relay server closed for frequency" << frequency;
        keepAliveTimer->stop();

        if (registry->isPendingRegistration(frequency)) {
            registry->removePendingRegistration(frequency);
        }

        if (registry->hasWavelength(frequency)) {
            qDebug() << "[Joiner] Removing wavelength" << frequency << "due to socket disconnect";
            const QString activeFreq = registry->getActiveWavelength();
            registry->removeWavelength(frequency);
            if (activeFreq == frequency) {
                registry->setActiveWavelength("-1");
                emit wavelengthLeft(frequency);
            } else {
                emit wavelengthClosed(frequency);
            }
        }

        socket->deleteLater();
        keepAliveTimer->deleteLater();
        delete connectedCallbackExecuted;
    };

    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this, socket, frequency, keepAliveTimer, registry](const QAbstractSocket::SocketError error) {
                qDebug() << "[Joiner] WebSocket error:" << socket->errorString() << "(Code:" << error << ")";
                keepAliveTimer->stop();

                if (registry->isPendingRegistration(frequency)) {
                    registry->removePendingRegistration(frequency);
                }

                emit connectionError("Błąd połączenia: " + socket->errorString());
            });

    connect(socket, &QWebSocket::disconnected, this, disconnectHandler);

    connect(socket, &QWebSocket::connected, this, [this, socket, frequency, password, clientId, keepAliveTimer, connectedCallbackExecuted, joinResultHandler, registry]() {
        qDebug() << "[Joiner] Connected to relay server for joining frequency" << frequency;

        if (*connectedCallbackExecuted) {
            qDebug() << "[Joiner] Connected callback already executed, ignoring";
            return;
        }
        *connectedCallbackExecuted = true;

        WavelengthInfo initialInfo;
        initialInfo.frequency = frequency;
        initialInfo.isPasswordProtected = !password.isEmpty();
        initialInfo.hostId = "";
        initialInfo.isHost = false;
        initialInfo.socket = socket;
        registry->addWavelength(frequency, initialInfo);

        qDebug() << "[Joiner] Setting socket message handlers for" << frequency;
        WavelengthMessageProcessor::getInstance()->setSocketMessageHandlers(socket, frequency);

        qDebug() << "[Joiner] Connecting temporary handler for join_result";
        connect(socket, &QWebSocket::textMessageReceived, this, joinResultHandler);

        connect(keepAliveTimer, &QTimer::timeout, socket, [socket](){
            if (socket->isValid()) {
                socket->ping();
            }
        });

        QJsonObject joinData;
        joinData["type"] = "join_wavelength";
        joinData["frequency"] = frequency;
        if (!password.isEmpty()) {
            joinData["password"] = password;
        }
        joinData["clientId"] = clientId;
        const QJsonDocument doc(joinData);
        const QString message = doc.toJson(QJsonDocument::Compact);
        qDebug() << "[Joiner] Sending join message:" << message;
        socket->sendTextMessage(message);
    });

    const QString address = config->GetRelayServerAddress();
    const int port = config->GetRelayServerPort();
    const QUrl url(QString("ws://%1:%2").arg(address).arg(port));
    qDebug() << "[Joiner] Opening WebSocket connection to URL for joining:" << url.toString();
    socket->open(url);

    return {true, QString()};
}
