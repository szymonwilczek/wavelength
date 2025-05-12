#ifndef WAVELENGTH_JOINER_H
#define WAVELENGTH_JOINER_H

#include <QObject>

/**
 * @brief Represents the result of an attempt to join a wavelength.
 */
struct JoinResult {
    /** @brief True if the join attempt was successful, false otherwise. */
    bool success;
    /** @brief If success is false, contains a string describing the reason for failure (e.g., "Password required", "Invalid password", "WAVELENGTH UNAVAILABLE"). */
    QString error_reason;
};

/**
 * @brief Singleton class responsible for initiating the process of joining an existing wavelength.
 *
 * This class handles connecting to the relay server, sending an authentication/join request
 * for a specific frequency (wavelength), and managing the WebSocket connection. It interacts
 * with WavelengthRegistry, WavelengthConfig, AuthenticationManager, MessageHandler, and
 * WavelengthMessageProcessor. It emits signals indicating success (wavelengthJoined),
 * failure (connectionError, authenticationFailed), or disconnection (wavelengthClosed, wavelengthLeft).
 */
class WavelengthJoiner final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the WavelengthJoiner.
     * @return Pointer to the singleton WavelengthJoiner instance.
     */
    static WavelengthJoiner *GetInstance() {
        static WavelengthJoiner instance;
        return &instance;
    }

    /**
     * @brief Attempts to join an existing wavelength hosted on the relay server.
     * Checks if the wavelength is already joined or pending registration.
     * Creates a new WebSocket connection, connects signals for connection events, errors,
     * and the join result message. Sends the join request upon successful connection.
     * Manages a keep-alive timer for the connection.
     * @param frequency The frequency identifier of the wavelength to join.
     * @param password Optional password required to join the wavelength.
     * @return A JoinResult struct indicating immediate success (already joined) or failure (pending),
     *         or {true, QString()} if the connection process was initiated. Asynchronous results
     *         are communicated via signals.
     */
    JoinResult JoinWavelength(QString frequency, const QString &password = QString());

signals:
    /**
     * @brief Emitted when the user successfully joins the specified wavelength.
     * @param frequency The frequency identifier of the joined wavelength.
     */


    void wavelengthJoined(QString frequency);

    /**
     * @brief Emitted if the WebSocket connection for the wavelength is closed unexpectedly
     *        (e.g., after joining).
     * @param frequency The frequency identifier of the wavelength whose connection closed.
     */
    void wavelengthClosed(QString frequency);

    /**
     * @brief Emitted if the join attempt fails due to incorrect or missing password.
     * @param frequency The frequency identifier for which authentication failed.
     */
    void authenticationFailed(QString frequency);

    /**
     * @brief Emitted if an error occurs during the connection or join process.
     * @param error_message A description of the error.
     */
    void connectionError(const QString &error_message);

    /**
     * @brief Emitted when a regular chat message is received for the joined frequency.
     * Note: This signal seems misplaced here and might be better handled solely by WavelengthMessageProcessor/Service.
     * @param frequency The frequency the message belongs to.
     * @param formatted_message The HTML-formatted message string.
     */
    void messageReceived(QString frequency, const QString &formatted_message);

    /**
     * @brief Emitted when the user explicitly leaves or is disconnected from the active wavelength.
     * @param frequency The frequency identifier of the wavelength that was left.
     */
    void wavelengthLeft(QString frequency);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional parent QObject.
     */
    explicit WavelengthJoiner(QObject *parent = nullptr) : QObject(parent) {
    }

    /**
     * @brief Private destructor.
     */
    ~WavelengthJoiner() override = default;

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    WavelengthJoiner(const WavelengthJoiner &) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    WavelengthJoiner &operator=(const WavelengthJoiner &) = delete;
};

#endif // WAVELENGTH_JOINER_H
