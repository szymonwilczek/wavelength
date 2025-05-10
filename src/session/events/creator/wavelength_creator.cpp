#include "wavelength_creator.h"

#include "../../../auth/authentication_manager.h"

bool WavelengthCreator::CreateWavelength(QString frequency, bool is_password_protected, const QString &password) {
    WavelengthRegistry *registry = WavelengthRegistry::GetInstance();
    const WavelengthConfig *config = WavelengthConfig::GetInstance();

    if (registry->HasWavelength(frequency)) {
        qDebug() << "[CREATOR] Wavelength" << frequency << "already exists locally.";
        return false;
    }

    if (registry->IsPendingRegistration(frequency)) {
        qDebug() << "[CREATOR] Wavelength" << frequency << "registration is already pending.";
        return false;
    }

    registry->AddPendingRegistration(frequency);
    QString host_id = AuthenticationManager::GetInstance()->GenerateClientId();
    // ReSharper disable once CppDFAUnreadVariable
    // ReSharper disable once CppDFAUnusedValue
    auto connected_callback_executed = new bool(false);
    // ReSharper disable once CppDFAUnreadVariable
    // ReSharper disable once CppDFAUnusedValue
    auto keep_alive_timer = new QTimer(this);

    auto socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

    auto result_handler = [this, frequency, socket, keep_alive_timer, registry](const QString &message) {
        bool ok = false;
        QJsonObject message_object = MessageHandler::GetInstance()->ParseMessage(message, &ok);
        if (!ok) {
            qDebug() << "[CREATOR] Failed to parse JSON message in RegisterResultHandler.";
            return;
        }

        const QString message_type = MessageHandler::GetInstance()->GetMessageType(message_object);

        if (message_type == "register_result") {
            disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);

            const bool success = message_object["success"].toBool();

            registry->RemovePendingRegistration(frequency);

            if (success) {
                WavelengthInfo info = registry->GetWavelengthInfo(frequency);
                if (info.frequency.isEmpty()) {
                    qWarning() << "[CREATOR] WavelengthInfo not found after successful registration for" << frequency;
                    info.frequency = frequency;
                    info.host_id = message_object["hostId"].toString();
                    info.is_host = true;
                    info.socket = socket;
                    registry->AddWavelength(frequency, info);
                } else {
                    info.host_id = message_object["hostId"].toString();
                    registry->UpdateWavelength(frequency, info);
                }

                registry->SetActiveWavelength(frequency);
                keep_alive_timer->start(WavelengthConfig::GetInstance()->GetKeepAliveInterval());
                emit wavelengthCreated(frequency);
            } else {
                const QString error_message = message_object["error"].toString("Unknown error");
                qDebug() << "[CREATOR] Failed to register wavelength:" << error_message;
                emit connectionError(error_message);
                keep_alive_timer->stop();
                socket->close();
            }
        } else {
            qDebug() << "[CREATOR] Received unexpected message type in RegisterResultHandler:" << message_type;
        }
    };

    auto disconnect_handler = [this, frequency, socket, keep_alive_timer, connected_callback_executed, registry]() {
        keep_alive_timer->stop();

        if (registry->IsPendingRegistration(frequency)) {
            registry->RemovePendingRegistration(frequency);
        }

        if (registry->HasWavelength(frequency)) {
            const QString activeFreq = registry->GetActiveWavelength();
            registry->RemoveWavelength(frequency);
            if (activeFreq == frequency) {
                registry->SetActiveWavelength("-1");
            }
            emit wavelengthClosed(frequency);
        }

        socket->deleteLater();
        keep_alive_timer->deleteLater();
        delete connected_callback_executed;
    };

    connect(socket, &QWebSocket::disconnected, this, disconnect_handler);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this, socket, frequency, registry](const QAbstractSocket::SocketError error) {
                qDebug() << "[CREATOR] WebSocket error for wavelength"
                        << frequency << ":" << socket->errorString() << "(Code:" << error << ")";

                if (registry->IsPendingRegistration(frequency)) {
                    registry->RemovePendingRegistration(frequency);
                }

                const QString errorMsg = socket->errorString();
                emit connectionError(errorMsg);
            });

    connect(socket, &QWebSocket::connected, this, [this, socket, frequency, is_password_protected,
                password, host_id, keep_alive_timer, connected_callback_executed, result_handler, registry]() {
                if (*connected_callback_executed) {
                    return;
                }
                *connected_callback_executed = true;

                WavelengthInfo initial_info;
                initial_info.frequency = frequency;
                initial_info.is_password_protected = is_password_protected;
                initial_info.password = password;
                initial_info.host_id = host_id;
                initial_info.is_host = true;
                initial_info.socket = socket;
                registry->AddWavelength(frequency, initial_info);

                WavelengthMessageProcessor::GetInstance()->SetSocketMessageHandlers(socket, frequency);

                connect(socket, &QWebSocket::textMessageReceived, this, result_handler);

                connect(keep_alive_timer, &QTimer::timeout, socket, [socket]() {
                    if (socket->isValid()) {
                        socket->ping();
                    }
                });

                const MessageHandler *message_handler = MessageHandler::GetInstance();
                const QJsonObject register_request = message_handler->CreateRegisterRequest(
                    frequency, is_password_protected, password, host_id);
                const QJsonDocument document(register_request);
                socket->sendTextMessage(document.toJson(QJsonDocument::Compact));
            });

    const QString address = config->GetRelayServerAddress();
    const int port = config->GetRelayServerPort();
    const QUrl url(QString("ws://%1:%2").arg(address).arg(port));
    socket->open(url);

    return true;
}
