#ifndef AUTHENTICATION_MANAGER_H
#define AUTHENTICATION_MANAGER_H

#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QString>

class AuthenticationManager final : public QObject {
    Q_OBJECT

public:
    static AuthenticationManager* GetInstance() {
        static AuthenticationManager instance;
        return &instance;
    }

    static QString GenerateClientId();
    
    static QString GenerateSessionToken();

    bool VerifyPassword(const QString &frequency, const QString& provided_password);

    void RegisterPassword(const QString &frequency, const QString& password);

    void RemovePassword(const QString &frequency);

    static QString CreateAuthResponse(bool success, const QString& error_message = QString());

    bool StoreSession(const QString &frequency, const QString& client_id, const QString& session_token);

    bool ValidateSession(const QString& session_token, const QString &frequency);

    void DeactivateSession(const QString& session_token);

    void DeactivateClientSessions(const QString& client_id);

    void DeactivateFrequencySessions(const QString &frequency);

    void CleanupExpiredSessions();

private:
    struct SessionInfo {
        QString client_id;
        QString frequency;
        QDateTime timestamp;
        bool is_active;
    };

    explicit AuthenticationManager(QObject* parent = nullptr) : QObject(parent) {}
    ~AuthenticationManager() override {}

    AuthenticationManager(const AuthenticationManager&) = delete;
    AuthenticationManager& operator=(const AuthenticationManager&) = delete;

    QMap<QString, QString> wavelength_passwords_;
    QMap<QString, SessionInfo> sessions_;
};

#endif // AUTHENTICATION_MANAGER_H