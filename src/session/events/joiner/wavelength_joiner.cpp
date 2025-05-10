#include "wavelength_joiner.h"

#include "../../../app/wavelength_config.h"
#include "../../../auth/authentication_manager.h"
#include "../../../chat/messages/handler/message_handler.h"
#include "../../../chat/messages/services/message_processor.h"
#include "../../../storage/wavelength_registry.h"

JoinResult WavelengthJoiner::JoinWavelength(QString frequency, const QString &password) {
    WavelengthRegistry *registry = WavelengthRegistry::GetInstance();
    const WavelengthConfig *config = WavelengthConfig::GetInstance();

    if (registry->HasWavelength(frequency)) {
        qDebug() << "[JOINER] Already joined wavelength" << frequency;
        registry->SetActiveWavelength(frequency);
        emit wavelengthJoined(frequency);
        return {true, QString()};
    }

    if (registry->IsPendingRegistration(frequency)) {
        qDebug() << "[JOINER] Wavelength" << frequency << "registration is already pending.";
        return {false, "WAVELENGTH UNAVAILABLE"};
    }

    registry->AddPendingRegistration(frequency);
    QString client_id = AuthenticationManager::GetInstance()->GenerateClientId();
    // ReSharper disable once CppDFAUnreadVariable
    // ReSharper disable once CppDFAUnusedValue
    auto connected_callback_executed = new bool(false);
    // ReSharper disable once CppDFAUnreadVariable
    // ReSharper disable once CppDFAUnusedValue
    auto keep_alive_timer = new QTimer(this);

    auto socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

    auto result_handler = [this, frequency, socket, keep_alive_timer, registry](const QString &message) {
        bool ok;
        QJsonObject message_object = MessageHandler::GetInstance()->ParseMessage(message, &ok);
        if (!ok) {
            return;
        }

        const QString message_type = MessageHandler::GetInstance()->GetMessageType(message_object);

        if (message_type == "join_result") {
            disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);

            const bool success = message_object["success"].toBool();
            const QString error_message = message_object["error"].toString();

            registry->RemovePendingRegistration(frequency);

            if (!success) {
                if (error_message == "Password required" || error_message == "Invalid password") {
                    emit connectionError(error_message == "Password required"
                                             ? "Password required"
                                             : "Incorrect password");
                    emit authenticationFailed(frequency);
                } else {
                    emit connectionError("Wavelength is unavailable or there was en error: " + error_message);
                    qDebug() << "[JOINER] Frequency" << frequency << "is unavailable or error occurred:" <<
                            error_message;
                }
                keep_alive_timer->stop();
                socket->close();
                return;
            }

            WavelengthInfo info = registry->GetWavelengthInfo(frequency);
            if (info.frequency.isEmpty()) {
                qWarning() << "[JOINER] WavelengthInfo not found after successful join for" << frequency;
                info.frequency = frequency;
                info.host_id = message_object["hostId"].toString();
                info.is_host = false;
                info.socket = socket;
                registry->AddWavelength(frequency, info);
            } else {
                info.host_id = message_object["hostId"].toString();
                registry->UpdateWavelength(frequency, info);
            }

            registry->SetActiveWavelength(frequency);
            keep_alive_timer->start(WavelengthConfig::GetInstance()->GetKeepAliveInterval());
            emit wavelengthJoined(frequency);
        } else {
            qDebug() << "[JOINER] Received unexpected message type in JoinResultHandler:" << message_type;
        }
    };

    auto disconnect_handler = [this, frequency, socket, keep_alive_timer, connected_callback_executed, registry]() {
        keep_alive_timer->stop();

        if (registry->IsPendingRegistration(frequency)) {
            registry->RemovePendingRegistration(frequency);
        }

        if (registry->HasWavelength(frequency)) {
            const QString active_frequency = registry->GetActiveWavelength();
            registry->RemoveWavelength(frequency);
            if (active_frequency == frequency) {
                registry->SetActiveWavelength("-1");
                emit wavelengthLeft(frequency);
            } else {
                emit wavelengthClosed(frequency);
            }
        }

        socket->deleteLater();
        keep_alive_timer->deleteLater();
        delete connected_callback_executed;
    };

    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this, socket, frequency, keep_alive_timer, registry](const QAbstractSocket::SocketError error) {
                qDebug() << "[JOINER] WebSocket error:" << socket->errorString() << "(Code:" << error << ")";
                keep_alive_timer->stop();

                if (registry->IsPendingRegistration(frequency)) {
                    registry->RemovePendingRegistration(frequency);
                }

                emit connectionError("Connection error: " + socket->errorString());
            });

    connect(socket, &QWebSocket::disconnected, this, disconnect_handler);

    connect(socket, &QWebSocket::connected, this,
            [this, socket, frequency, password, client_id, keep_alive_timer, connected_callback_executed, result_handler
                , registry]() {
                if (*connected_callback_executed) {
                    return;
                }
                *connected_callback_executed = true;

                WavelengthInfo initial_info;
                initial_info.frequency = frequency;
                initial_info.is_password_protected = !password.isEmpty();
                initial_info.host_id = "";
                initial_info.is_host = false;
                initial_info.socket = socket;
                registry->AddWavelength(frequency, initial_info);

                MessageProcessor::GetInstance()->SetSocketMessageHandlers(socket, frequency);

                connect(socket, &QWebSocket::textMessageReceived, this, result_handler);

                connect(keep_alive_timer, &QTimer::timeout, socket, [socket]() {
                    if (socket->isValid()) {
                        socket->ping();
                    }
                });

                QJsonObject join_data;
                join_data["type"] = "join_wavelength";
                join_data["frequency"] = frequency;
                if (!password.isEmpty()) {
                    join_data["password"] = password;
                }
                join_data["clientId"] = client_id;
                const QJsonDocument document(join_data);
                const QString message = document.toJson(QJsonDocument::Compact);
                socket->sendTextMessage(message);
            });

    const QString address = config->GetRelayServerAddress();
    const int port = config->GetRelayServerPort();
    const QUrl url(QString("ws://%1:%2").arg(address).arg(port));
    socket->open(url);

    return {true, QString()};
}
