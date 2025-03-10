#ifndef AUTHENTICATION_MANAGER_H
#define AUTHENTICATION_MANAGER_H

#include <QObject>
#include <QString>
#include <QUuid>
#include <QMap>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>

class AuthenticationManager : public QObject {
    Q_OBJECT

public:
    static AuthenticationManager* getInstance() {
        static AuthenticationManager instance;
        return &instance;
    }

    QString generateClientId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    QString generateSessionToken() {
        QByteArray tokenData = QUuid::createUuid().toByteArray() + 
                              QByteArray::number(QDateTime::currentMSecsSinceEpoch());
        return QString(QCryptographicHash::hash(tokenData, QCryptographicHash::Sha256).toHex());
    }

    bool verifyPassword(int frequency, const QString& providedPassword) {
        if (!m_wavelengthPasswords.contains(frequency)) {
            qDebug() << "No password stored for frequency" << frequency;
            return false;
        }
        
        QString storedPassword = m_wavelengthPasswords[frequency];
        bool isValid = (providedPassword == storedPassword);
        
        qDebug() << "Password verification for frequency" << frequency << ":" 
                << (isValid ? "successful" : "failed");
        
        return isValid;
    }

    void registerPassword(int frequency, const QString& password) {
        m_wavelengthPasswords[frequency] = password;
        qDebug() << "Password registered for frequency" << frequency;
    }

    void removePassword(int frequency) {
        if (m_wavelengthPasswords.contains(frequency)) {
            m_wavelengthPasswords.remove(frequency);
            qDebug() << "Password removed for frequency" << frequency;
        }
    }

    QString createAuthResponse(bool success, const QString& errorMessage = QString()) {
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

    bool storeSession(int frequency, const QString& clientId, const QString& sessionToken) {
        SessionInfo info;
        info.clientId = clientId;
        info.frequency = frequency;
        info.timestamp = QDateTime::currentDateTime();
        info.isActive = true;
        
        m_sessions[sessionToken] = info;
        qDebug() << "Session stored for client" << clientId << "on frequency" << frequency;
        
        return true;
    }

    bool validateSession(const QString& sessionToken, int frequency) {
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
        QDateTime expiryTime = info.timestamp.addSecs(86400);
        if (QDateTime::currentDateTime() > expiryTime) {
            qDebug() << "Session has expired";
            m_sessions[sessionToken].isActive = false;
            return false;
        }
        
        return true;
    }

    void deactivateSession(const QString& sessionToken) {
        if (m_sessions.contains(sessionToken)) {
            m_sessions[sessionToken].isActive = false;
            qDebug() << "Session deactivated:" << sessionToken;
        }
    }

    void deactivateClientSessions(const QString& clientId) {
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
            if (it.value().clientId == clientId) {
                it.value().isActive = false;
                qDebug() << "Deactivated session for client:" << clientId;
            }
        }
    }

    void deactivateFrequencySessions(int frequency) {
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
            if (it.value().frequency == frequency) {
                it.value().isActive = false;
                qDebug() << "Deactivated session for frequency:" << frequency;
            }
        }
    }

    void cleanupExpiredSessions() {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime expiryThreshold = now.addSecs(-86400); // 24 hours ago
        
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

private:
    struct SessionInfo {
        QString clientId;
        int frequency;
        QDateTime timestamp;
        bool isActive;
    };

    AuthenticationManager(QObject* parent = nullptr) : QObject(parent) {}
    ~AuthenticationManager() {}

    AuthenticationManager(const AuthenticationManager&) = delete;
    AuthenticationManager& operator=(const AuthenticationManager&) = delete;

    QMap<int, QString> m_wavelengthPasswords;
    QMap<QString, SessionInfo> m_sessions;
};

#endif // AUTHENTICATION_MANAGER_H