#ifndef AUTHENTICATION_MANAGER_H
#define AUTHENTICATION_MANAGER_H

#include <QObject>
#include <QString>
#include <QCryptographicHash>
#include <QJsonDocument>

class AuthenticationManager final : public QObject {
    Q_OBJECT

public:
    static AuthenticationManager* getInstance() {
        static AuthenticationManager instance;
        return &instance;
    }

    static QString generateClientId();
    
    static QString generateSessionToken();

    bool verifyPassword(const QString &frequency, const QString& providedPassword);

    void registerPassword(const QString &frequency, const QString& password);

    void removePassword(const QString &frequency);

    static QString createAuthResponse(bool success, const QString& errorMessage = QString());

    bool storeSession(const QString &frequency, const QString& clientId, const QString& sessionToken);

    bool validateSession(const QString& sessionToken, const QString &frequency);

    void deactivateSession(const QString& sessionToken);

    void deactivateClientSessions(const QString& clientId);

    void deactivateFrequencySessions(const QString &frequency);

    void cleanupExpiredSessions();

private:
    struct SessionInfo {
        QString clientId;
        QString frequency;
        QDateTime timestamp;
        bool isActive;
    };

    explicit AuthenticationManager(QObject* parent = nullptr) : QObject(parent) {}
    ~AuthenticationManager() override {}

    AuthenticationManager(const AuthenticationManager&) = delete;
    AuthenticationManager& operator=(const AuthenticationManager&) = delete;

    // ReSharper disable once CppDFANotInitializedField
    QMap<QString, QString> m_wavelengthPasswords;
    QMap<QString, SessionInfo> m_sessions;
};

#endif // AUTHENTICATION_MANAGER_H