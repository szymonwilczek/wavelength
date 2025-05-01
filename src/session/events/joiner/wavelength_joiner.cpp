#include "wavelength_joiner.h"

#include "../../../auth/authentication_manager.h"

JoinResult WavelengthJoiner::JoinWavelength(QString frequency, const QString &password) {
    WavelengthRegistry* registry = WavelengthRegistry::GetInstance();
    const WavelengthConfig* config = WavelengthConfig::GetInstance();

    if (registry->HasWavelength(frequency)) {
        qDebug() << "Already joined wavelength" << frequency;
        registry->SetActiveWavelength(frequency);
        emit wavelengthJoined(frequency);
        return {true, QString()};
    }

    if (registry->IsPendingRegistration(frequency)) {
        qDebug() << "Wavelength" << frequency << "registration is already pending";
        return {false, "WAVELENGTH UNAVAILABLE"};
    }

    registry->AddPendingRegistration(frequency);
    QString client_id = AuthenticationManager::GetInstance()->GenerateClientId();
    auto connected_callback_executed = new bool(false);
    auto keep_alive_timer = new QTimer(this);

    auto socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

    auto result_handler = [this, frequency, socket, keep_alive_timer, registry](const QString& message) {
        qDebug() << "[Joiner] JoinResultHandler received message:" << message;

        bool ok;
        QJsonObject message_object = MessageHandler::GetInstance()->ParseMessage(message, &ok);
        if (!ok) {
            qDebug() << "[Joiner] Failed to parse message in JoinResultHandler";
            return;
        }

        const QString message_type = MessageHandler::GetInstance()->GetMessageType(message_object);
        qDebug() << "[Joiner] Message type in JoinResultHandler:" << message_type;

        if (message_type == "join_result") {
            disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);

            const bool success = message_object["success"].toBool();
            const QString error_message = message_object["error"].toString();

            registry->RemovePendingRegistration(frequency);

            if (!success) {
                if (error_message == "Password required" || error_message == "Invalid password") {
                    emit connectionError(error_message == "Password required" ? "Ta częstotliwość wymaga hasła" : "Nieprawidłowe hasło");
                    qDebug() << "[Joiner]" << (error_message == "Password required" ? "Password required" : "Invalid password") << "for frequency" << frequency;
                    emit authenticationFailed(frequency);
                } else {
                    emit connectionError("Częstotliwość jest niedostępna lub wystąpił błąd: " + error_message);
                    qDebug() << "[Joiner] Frequency" << frequency << "is unavailable or error occurred:" << error_message;
                }
                keep_alive_timer->stop();
                socket->close();
                return;
            }

                qDebug() << "[Joiner] Join successful for frequency" << frequency;
                WavelengthInfo info = registry->GetWavelengthInfo(frequency);
                if (info.frequency.isEmpty()) {
                    qWarning() << "[Joiner] WavelengthInfo not found after successful join for" << frequency;
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
                qDebug() << "[Joiner] Keep-alive timer started for" << frequency;
                emit wavelengthJoined(frequency);

        } else {
            qDebug() << "[Joiner] Received unexpected message type in JoinResultHandler:" << message_type;
        }
    };

    auto disconnect_handler = [this, frequency, socket, keep_alive_timer, connected_callback_executed, registry]() {
        qDebug() << "[Joiner] Connection to relay server closed for frequency" << frequency;
        keep_alive_timer->stop();

        if (registry->IsPendingRegistration(frequency)) {
            registry->RemovePendingRegistration(frequency);
        }

        if (registry->HasWavelength(frequency)) {
            qDebug() << "[Joiner] Removing wavelength" << frequency << "due to socket disconnect";
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
                qDebug() << "[Joiner] WebSocket error:" << socket->errorString() << "(Code:" << error << ")";
                keep_alive_timer->stop();

                if (registry->IsPendingRegistration(frequency)) {
                    registry->RemovePendingRegistration(frequency);
                }

                emit connectionError("Błąd połączenia: " + socket->errorString());
            });

    connect(socket, &QWebSocket::disconnected, this, disconnect_handler);

    connect(socket, &QWebSocket::connected, this, [this, socket, frequency, password, client_id, keep_alive_timer, connected_callback_executed, result_handler, registry]() {
        qDebug() << "[Joiner] Connected to relay server for joining frequency" << frequency;

        if (*connected_callback_executed) {
            qDebug() << "[Joiner] Connected callback already executed, ignoring";
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

        qDebug() << "[Joiner] Setting socket message handlers for" << frequency;
        WavelengthMessageProcessor::GetInstance()->SetSocketMessageHandlers(socket, frequency);

        qDebug() << "[Joiner] Connecting temporary handler for join_result";
        connect(socket, &QWebSocket::textMessageReceived, this, result_handler);

        connect(keep_alive_timer, &QTimer::timeout, socket, [socket](){
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
