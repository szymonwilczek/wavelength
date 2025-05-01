#include "authentication_manager.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>

QString AuthenticationManager::GenerateClientId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString AuthenticationManager::GenerateSessionToken() {
    const QByteArray token_data = QUuid::createUuid().toByteArray() +
                          QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    return {QCryptographicHash::hash(token_data, QCryptographicHash::Sha256).toHex()};
}

bool AuthenticationManager::VerifyPassword(const QString &frequency, const QString& provided_password) {
    if (!wavelength_passwords_.contains(frequency)) {
        qDebug() << "No password stored for frequency" << frequency;
        return false;
    }

    const QString stored_password = wavelength_passwords_[frequency];
    const bool is_valid = provided_password == stored_password;

    qDebug() << "Password verification for frequency" << frequency << ":"
            << (is_valid ? "successful" : "failed");

    return is_valid;
}

void AuthenticationManager::RegisterPassword(const QString &frequency, const QString& password) {
    wavelength_passwords_.insert(frequency, password);
    qDebug() << "Password registered for frequency" << frequency;
}

void AuthenticationManager::RemovePassword(const QString &frequency) {
    if (wavelength_passwords_.contains(frequency)) {
        wavelength_passwords_.remove(frequency);
        qDebug() << "Password removed for frequency" << frequency;
    }
}

QString AuthenticationManager::CreateAuthResponse(const bool success, const QString& error_message) {
    QJsonObject response;
    response["type"] = "authResult";
    response["success"] = success;

    if (!error_message.isEmpty()) {
        response["errorMessage"] = error_message;
    }

    if (success) {
        response["sessionToken"] = GenerateSessionToken();
    }

    return QJsonDocument(response).toJson(QJsonDocument::Compact);
}

bool AuthenticationManager::StoreSession(const QString &frequency, const QString& client_id, const QString& session_token) {
    SessionInfo info;
    info.client_id = client_id;
    info.frequency = frequency;
    info.timestamp = QDateTime::currentDateTime();
    info.is_active = true;

    sessions_[session_token] = info;
    qDebug() << "Session stored for client" << client_id << "on frequency" << frequency;

    return true;
}

bool AuthenticationManager::ValidateSession(const QString& session_token, const QString &frequency) {
    if (!sessions_.contains(session_token)) {
        qDebug() << "Session token not found";
        return false;
    }

    const SessionInfo& info = sessions_[session_token];

    // Check if session is for the correct frequency
    if (info.frequency != frequency) {
        qDebug() << "Session frequency mismatch:" << info.frequency << "vs" << frequency;
        return false;
    }

    // Check if session is active
    if (!info.is_active) {
        qDebug() << "Session is no longer active";
        return false;
    }

    // Check if session has expired (24 hours)
    if (const QDateTime expiry_time = info.timestamp.addSecs(86400); QDateTime::currentDateTime() > expiry_time) {
        qDebug() << "Session has expired";
        sessions_[session_token].is_active = false;
        return false;
    }

    return true;
}

void AuthenticationManager::DeactivateSession(const QString& session_token) {
    if (sessions_.contains(session_token)) {
        sessions_[session_token].is_active = false;
        qDebug() << "Session deactivated:" << session_token;
    }
}

void AuthenticationManager::DeactivateClientSessions(const QString& client_id) {
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it.value().client_id == client_id) {
            it.value().is_active = false;
            qDebug() << "Deactivated session for client:" << client_id;
        }
    }
}

void AuthenticationManager::DeactivateFrequencySessions(const QString &frequency) {
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it.value().frequency == frequency) {
            it.value().is_active = false;
            qDebug() << "Deactivated session for frequency:" << frequency;
        }
    }
}

void AuthenticationManager::CleanupExpiredSessions() {
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime expiry_threshold = now.addSecs(-86400); // 24 hours ago

    QList<QString> tokens_to_remove;

    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it.value().timestamp < expiry_threshold) {
            tokens_to_remove.append(it.key());
        }
    }

    for (const QString& token : tokens_to_remove) {
        sessions_.remove(token);
    }

    if (!tokens_to_remove.isEmpty()) {
        qDebug() << "Cleaned up" << tokens_to_remove.size() << "expired sessions";
    }
}
