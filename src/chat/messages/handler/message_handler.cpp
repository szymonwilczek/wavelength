#include "message_handler.h"

bool MessageHandler::SendSystemCommand(QWebSocket *socket, const QString &command, const QJsonObject &params) {
    if (!socket || !socket->isValid()) {
        qDebug() << "Cannot send command - socket is invalid";
        return false;
    }

    QJsonObject command_object;
    command_object["type"] = command;

    // Add all parameters from the params object
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        command_object[it.key()] = it.value();
    }

    const QJsonDocument doc(command_object);
    socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

    return true;
}

void MessageHandler::MarkMessageAsProcessed(const QString &message_id) {
    if (!message_id.isEmpty()) {
        processed_message_ids_.insert(message_id);

        // Keep the processed message cache from growing too large
        if (processed_message_ids_.size() > kMaxCachedMessageIds) {
            // Remove approximately 20% of the oldest messages
            constexpr int to_remove = kMaxCachedMessageIds / 5;
            auto it = processed_message_ids_.begin();
            for (int i = 0; i < to_remove && it != processed_message_ids_.end(); ++i) {
                it = processed_message_ids_.erase(it);
            }
        }
    }
}

QJsonObject MessageHandler::ParseMessage(const QString &message, bool *ok) {
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());
    if (document.isNull() || !document.isObject()) {
        if (ok) *ok = false;
        qDebug() << "Failed to parse message JSON";
        return QJsonObject();
    }

    if (ok) *ok = true;
    return document.object();
}

QJsonObject MessageHandler::CreateAuthRequest(const QString &frequency, const QString &password,
    const QString &client_id) {
    QJsonObject auth_object;
    auth_object["type"] = "auth";
    auth_object["frequency"] = frequency;
    auth_object["password"] = password;
    auth_object["clientId"] = client_id;
    return auth_object;
}

QJsonObject MessageHandler::CreateRegisterRequest(const QString &frequency, const bool is_password_protected,
    const QString &password, const QString &host_id) {
    QJsonObject register_object;
    register_object["type"] = "register_wavelength";
    register_object["frequency"] = frequency;
    register_object["isPasswordProtected"] = is_password_protected;
    register_object["password"] = password;
    register_object["hostId"] = host_id;
    return register_object;
}

QJsonObject MessageHandler::CreateLeaveRequest(const QString &frequency, const bool is_host) {
    QJsonObject leave_object;
    leave_object["type"] = is_host ? "close_wavelength" : "leave_wavelength";
    leave_object["frequency"] = frequency;
    return leave_object;
}
