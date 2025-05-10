#ifndef AUTHENTICATION_MANAGER_H
#define AUTHENTICATION_MANAGER_H

#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QString>

/**
 * @brief Manages authentication and session handling for Wavelength frequencies using a singleton pattern.
 *
 * This class is responsible for:
 * - Generating unique client IDs and session tokens.
 * - Registering and verifying passwords associated with specific frequencies.
 * - Creating authentication response messages (JSON).
 * - Storing, validating, and managing active client sessions.
 * - Deactivating sessions based on token, client ID, or frequency.
 * - Cleaning up expired sessions.
 *
 * Note: Password storage is currently in-memory and plain text, which is insecure for production environments.
 */
class AuthenticationManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the AuthenticationManager.
     * @return Pointer to the singleton AuthenticationManager instance.
     */
    static AuthenticationManager *GetInstance() {
        static AuthenticationManager instance;
        return &instance;
    }

    /**
     * @brief Generates a unique client identifier.
     * @return A unique client ID as a QString (UUID without braces).
     */
    static QString GenerateClientId();

    /**
     * @brief Generates a cryptographically secure session token.
     * Uses a combination of UUID and timestamp, hashed with SHA256.
     * @return A unique session token as a hexadecimal QString.
     */
    static QString GenerateSessionToken();

    /**
     * @brief Registers or updates the password for a specific frequency.
     * @param frequency The frequency identifier.
     * @param password The password to associate with the frequency.
     */
    void RegisterPassword(const QString &frequency, const QString &password);

    /**
     * @brief Verifies if the provided password matches the stored password for a given frequency.
     * @param frequency The frequency identifier.
     * @param provided_password The password attempt provided by the client.
     * @return True if the password is correct or if no password is set for the frequency, false otherwise.
     */
    bool VerifyPassword(const QString &frequency, const QString &provided_password);

    /**
     * @brief Removes the password associated with a specific frequency.
     * @param frequency The frequency identifier whose password should be removed.
     */
    void RemovePassword(const QString &frequency);

    /**
     * @brief Creates a JSON response string indicating the result of an authentication attempt.
     * Includes a session token if authentication was successful.
     * @param success True if authentication was successful, false otherwise.
     * @param error_message An optional error message to include if authentication failed.
     * @return A compact JSON string representing the authentication result.
     */
    static QString CreateAuthResponse(bool success, const QString &error_message = QString());

    /**
     * @brief Stores information about a newly established client session.
     * @param frequency The frequency the client connected to.
     * @param client_id The unique ID of the client.
     * @param session_token The unique token assigned to this session.
     * @return True if the session was stored successfully (currently always returns true).
     */
    bool StoreSession(const QString &frequency, const QString &client_id, const QString &session_token);

    /**
     * @brief Validates an existing session token for a specific frequency.
     * Checks if the token exists, belongs to the correct frequency, is active, and has not expired (24h validity).
     * @param session_token The session token to validate.
     * @param frequency The frequency the session should be associated with.
     * @return True if the session is valid, false otherwise.
     */
    bool ValidateSession(const QString &session_token, const QString &frequency);

    /**
     * @brief Marks a specific session as inactive.
     * @param session_token The token of the session to deactivate.
     */
    void DeactivateSession(const QString &session_token);

    /**
     * @brief Marks all sessions associated with a specific client ID as inactive.
     * @param client_id The ID of the client whose sessions should be deactivated.
     */
    void DeactivateClientSessions(const QString &client_id);

    /**
     * @brief Marks all sessions associated with a specific frequency as inactive.
     * @param frequency The frequency identifier whose sessions should be deactivated.
     */
    void DeactivateFrequencySessions(const QString &frequency);

    /**
     * @brief Removes session information for sessions older than 24 hours.
     */
    void CleanupExpiredSessions();

private:
    /**
     * @brief Structure holding information about an active client session.
     */
    struct SessionInfo {
        QString client_id; ///< Unique identifier of the client holding the session.
        QString frequency; ///< The frequency the session is associated with.
        QDateTime timestamp; ///< Timestamp when the session was created or last validated.
        bool is_active; ///< Flag indicating if the session is currently active.
    };

    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional parent QObject.
     */
    explicit AuthenticationManager(QObject *parent = nullptr) : QObject(parent) {
    }

    /**
     * @brief Private destructor.
     */
    ~AuthenticationManager() override = default;

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    AuthenticationManager(const AuthenticationManager &) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    AuthenticationManager &operator=(const AuthenticationManager &) = delete;

    /**
     * @brief Map storing passwords associated with frequencies.
     * Key: Frequency identifier (QString).
     * Value: Password (QString).
     * @warning Passwords are stored in plain text in memory. This is insecure.
     */
    QMap<QString, QString> wavelength_passwords_{};

    /**
     * @brief Map storing active session information.
     * Key: Session token (QString).
     * Value: SessionInfo struct containing details about the session.
     */
    QMap<QString, SessionInfo> sessions_{};
};

#endif // AUTHENTICATION_MANAGER_H
