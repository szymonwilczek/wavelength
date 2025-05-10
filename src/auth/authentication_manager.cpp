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

void AuthenticationManager::RegisterPassword(const QString &frequency, const QString &password) {
    wavelength_passwords_.insert(frequency, password);
}

bool AuthenticationManager::VerifyPassword(const QString &frequency, const QString &provided_password) {
    if (!wavelength_passwords_.contains(frequency)) {
        qDebug() << "[AUTH MANAGER] No password stored for frequency" << frequency;
        return false;
    }

    const QString stored_password = wavelength_passwords_[frequency];
    const bool is_valid = provided_password == stored_password;

    return is_valid;
}

void AuthenticationManager::RemovePassword(const QString &frequency) {
    if (wavelength_passwords_.contains(frequency)) {
        wavelength_passwords_.remove(frequency);
    }
}

QString AuthenticationManager::CreateAuthResponse(const bool success, const QString &error_message) {
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

bool AuthenticationManager::StoreSession(const QString &frequency, const QString &client_id,
                                         const QString &session_token) {
    SessionInfo info;
    info.client_id = client_id;
    info.frequency = frequency;
    info.timestamp = QDateTime::currentDateTime();
    info.is_active = true;

    sessions_[session_token] = info;

    return true;
}

bool AuthenticationManager::ValidateSession(const QString &session_token, const QString &frequency) {
    if (!sessions_.contains(session_token)) {
        qDebug() << "[AUTH MANAGER] Session token not found";
        return false;
    }

    const SessionInfo &session_info = sessions_[session_token];

    if (session_info.frequency != frequency) {
        qDebug() << "[AUTH MANAGER] Session frequency mismatch:" << session_info.frequency << "vs" << frequency;
        return false;
    }

    if (!session_info.is_active) {
        qDebug() << "[AUTH MANAGER] Session is no longer active";
        return false;
    }

    if (const QDateTime expiry_time = session_info.timestamp.addSecs(86400);
        QDateTime::currentDateTime() > expiry_time) {
        qDebug() << "[AUTH MANAGER] Session has expired";
        sessions_[session_token].is_active = false;
        return false;
    }

    return true;
}

void AuthenticationManager::DeactivateSession(const QString &session_token) {
    if (sessions_.contains(session_token)) {
        sessions_[session_token].is_active = false;
    }
}

void AuthenticationManager::DeactivateClientSessions(const QString &client_id) {
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it.value().client_id == client_id) {
            it.value().is_active = false;
        }
    }
}

void AuthenticationManager::DeactivateFrequencySessions(const QString &frequency) {
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it.value().frequency == frequency) {
            it.value().is_active = false;
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

    for (const QString &token: tokens_to_remove) {
        sessions_.remove(token);
    }
}
