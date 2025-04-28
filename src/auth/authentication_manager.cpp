#include "authentication_manager.h"

#include <QJsonObject>

QString AuthenticationManager::generateClientId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString AuthenticationManager::generateSessionToken() {
    const QByteArray tokenData = QUuid::createUuid().toByteArray() +
                          QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    return {QCryptographicHash::hash(tokenData, QCryptographicHash::Sha256).toHex()};
}

bool AuthenticationManager::verifyPassword(const QString &frequency, const QString& providedPassword) {
    if (!m_wavelengthPasswords.contains(frequency)) {
        qDebug() << "No password stored for frequency" << frequency;
        return false;
    }

    const QString storedPassword = m_wavelengthPasswords[frequency];
    const bool isValid = providedPassword == storedPassword;

    qDebug() << "Password verification for frequency" << frequency << ":"
            << (isValid ? "successful" : "failed");

    return isValid;
}

void AuthenticationManager::registerPassword(const QString &frequency, const QString& password) {
    m_wavelengthPasswords.insert(frequency, password);
    qDebug() << "Password registered for frequency" << frequency;
}

void AuthenticationManager::removePassword(const QString &frequency) {
    if (m_wavelengthPasswords.contains(frequency)) {
        m_wavelengthPasswords.remove(frequency);
        qDebug() << "Password removed for frequency" << frequency;
    }
}

QString AuthenticationManager::createAuthResponse(const bool success, const QString& errorMessage) {
    QJsonObject response;
    response["type"] = "authResult";
    response["success"] = success;

    if (!errorMessage.isEmpty()) {
        response["errorMessage"] = errorMessage;
    }

    if (success) {
        response["sessionToken"] = generateSessionToken();
    }

    return QJsonDocument(response).toJson(QJsonDocument::Compact);
}

bool AuthenticationManager::storeSession(const QString &frequency, const QString& clientId, const QString& sessionToken) {
    SessionInfo info;
    info.clientId = clientId;
    info.frequency = frequency;
    info.timestamp = QDateTime::currentDateTime();
    info.isActive = true;

    m_sessions[sessionToken] = info;
    qDebug() << "Session stored for client" << clientId << "on frequency" << frequency;

    return true;
}

bool AuthenticationManager::validateSession(const QString& sessionToken, const QString &frequency) {
    if (!m_sessions.contains(sessionToken)) {
        qDebug() << "Session token not found";
        return false;
    }

    const SessionInfo& info = m_sessions[sessionToken];

    // Check if session is for the correct frequency
    if (info.frequency != frequency) {
        qDebug() << "Session frequency mismatch:" << info.frequency << "vs" << frequency;
        return false;
    }

    // Check if session is active
    if (!info.isActive) {
        qDebug() << "Session is no longer active";
        return false;
    }

    // Check if session has expired (24 hours)
    if (const QDateTime expiryTime = info.timestamp.addSecs(86400); QDateTime::currentDateTime() > expiryTime) {
        qDebug() << "Session has expired";
        m_sessions[sessionToken].isActive = false;
        return false;
    }

    return true;
}

void AuthenticationManager::deactivateSession(const QString& sessionToken) {
    if (m_sessions.contains(sessionToken)) {
        m_sessions[sessionToken].isActive = false;
        qDebug() << "Session deactivated:" << sessionToken;
    }
}

void AuthenticationManager::deactivateClientSessions(const QString& clientId) {
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.value().clientId == clientId) {
            it.value().isActive = false;
            qDebug() << "Deactivated session for client:" << clientId;
        }
    }
}

void AuthenticationManager::deactivateFrequencySessions(const QString &frequency) {
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.value().frequency == frequency) {
            it.value().isActive = false;
            qDebug() << "Deactivated session for frequency:" << frequency;
        }
    }
}

void AuthenticationManager::cleanupExpiredSessions() {
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime expiryThreshold = now.addSecs(-86400); // 24 hours ago

    QList<QString> tokensToRemove;

    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.value().timestamp < expiryThreshold) {
            tokensToRemove.append(it.key());
        }
    }

    for (const QString& token : tokensToRemove) {
        m_sessions.remove(token);
    }

    if (!tokensToRemove.isEmpty()) {
        qDebug() << "Cleaned up" << tokensToRemove.size() << "expired sessions";
    }
}
