#include "message_handler.h"

bool MessageHandler::sendSystemCommand(QWebSocket *socket, const QString &command, const QJsonObject &params) {
    if (!socket || !socket->isValid()) {
        qDebug() << "Cannot send command - socket is invalid";
        return false;
    }

    QJsonObject commandObj;
    commandObj["type"] = command;

    // Add all parameters from the params object
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        commandObj[it.key()] = it.value();
    }

    const QJsonDocument doc(commandObj);
    socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

    return true;
}

void MessageHandler::markMessageAsProcessed(const QString &messageId) {
    if (!messageId.isEmpty()) {
        m_processedMessageIds.insert(messageId);

        // Keep the processed message cache from growing too large
        if (m_processedMessageIds.size() > MAX_CACHED_MESSAGE_IDS) {
            // Remove approximately 20% of the oldest messages
            constexpr int toRemove = MAX_CACHED_MESSAGE_IDS / 5;
            auto it = m_processedMessageIds.begin();
            for (int i = 0; i < toRemove && it != m_processedMessageIds.end(); ++i) {
                it = m_processedMessageIds.erase(it);
            }
        }
    }
}

QJsonObject MessageHandler::parseMessage(const QString &message, bool *ok) {
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        if (ok) *ok = false;
        qDebug() << "Failed to parse message JSON";
        return QJsonObject();
    }

    if (ok) *ok = true;
    return doc.object();
}

QJsonObject MessageHandler::createAuthRequest(const QString &frequency, const QString &password,
    const QString &clientId) {
    QJsonObject authObj;
    authObj["type"] = "auth";
    authObj["frequency"] = frequency;
    authObj["password"] = password;
    authObj["clientId"] = clientId;
    return authObj;
}

QJsonObject MessageHandler::createRegisterRequest(const QString &frequency, const bool isPasswordProtected,
    const QString &password, const QString &hostId) {
    QJsonObject regObj;
    regObj["type"] = "register_wavelength";
    regObj["frequency"] = frequency;
    regObj["isPasswordProtected"] = isPasswordProtected;
    regObj["password"] = password;
    regObj["hostId"] = hostId;
    return regObj;
}

QJsonObject MessageHandler::createLeaveRequest(const QString &frequency, const bool isHost) {
    QJsonObject leaveObj;
    leaveObj["type"] = isHost ? "close_wavelength" : "leave_wavelength";
    leaveObj["frequency"] = frequency;
    return leaveObj;
}
