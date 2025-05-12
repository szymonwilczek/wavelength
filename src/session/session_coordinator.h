#ifndef WAVELENGTH_SESSION_COORDINATOR_H
#define WAVELENGTH_SESSION_COORDINATOR_H

#include <QObject>
#include <QDebug>

struct WavelengthInfo;

/**
 * @brief Singleton coordinator class acting as a facade for Wavelength session management.
 *
 * This class simplifies interactions with various Wavelength components (Creator, Joiner, Leaver,
 * MessageService, StateManager, Config, EventBroker) by providing a unified interface.
 * It handles the initialization sequence, connects signals between components, and forwards
 * events and actions. It also publishes events to the WavelengthEventBroker for decoupling.
 */
class SessionCoordinator final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the WavelengthSessionCoordinator.
     * @return Pointer to the singleton WavelengthSessionCoordinator instance.
     */
    static SessionCoordinator *GetInstance() {
        static SessionCoordinator instance;
        return &instance;
    }

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    SessionCoordinator(const SessionCoordinator &) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    SessionCoordinator &operator=(const SessionCoordinator &) = delete;

    /**
     * @brief Initializes the coordinator and its underlying components.
     * Connects signals between various Wavelength services and loads the application configuration.
     */
    void Initialize();

    /**
     * @brief Initiates the creation of a new wavelength.
     * Delegates the call to WavelengthCreator and registers the wavelength as joined locally upon initiation.
     * @param frequency The desired frequency name.
     * @param is_password_protected True if the wavelength should require a password.
     * @param password The password to set (if protected).
     * @return True if the creation process was initiated, false otherwise (e.g., already exists).
     */
    static bool CreateWavelength(const QString &frequency,
                                 bool is_password_protected, const QString &password);

    /**
     * @brief Initiates joining an existing wavelength.
     * Delegates the call to WavelengthJoiner and registers the wavelength as joined locally upon initiation.
     * @param frequency The frequency name to join.
     * @param password Optional password for the wavelength.
     * @return True if the join process was initiated, false otherwise (e.g., already joined).
     */
    static bool JoinWavelength(const QString &frequency, const QString &password = QString());

    /**
     * @brief Leaves the currently active wavelength.
     * Unregisters the wavelength locally and delegates the call to WavelengthLeaver.
     */
    static void LeaveWavelength();

    /**
     * @brief Closes a specific wavelength (only if the current user is the host).
     * Unregisters the wavelength locally and delegates the call to WavelengthLeaver.
     * @param frequency The frequency name to close.
     */
    static void CloseWavelength(const QString &frequency);

    /**
     * @brief Sends a text message to the currently active wavelength.
     * Delegates the call to WavelengthMessageService.
     * @param message The text message content.
     * @return True if the message was sent successfully, false otherwise.
     */
    static bool SendMessage(const QString &message);

    /**
     * @brief Sends a file to the currently active wavelength.
     * Delegates the call to WavelengthMessageService.
     * @param file_path The local path to the file.
     * @return True if the file sending a task was successfully queued, false otherwise.
     */
    static bool SendFile(const QString &file_path);

    /**
     * @brief Retrieves detailed information about a specific wavelength.
     * Delegates the call to WavelengthStateManager.
     * @param frequency The frequency identifier.
     * @param is_host Optional output parameter; set to true if the current user is the host.
     * @return A WavelengthInfo struct.
     */
    static WavelengthInfo GetWavelengthInfo(const QString &frequency, bool *is_host = nullptr);

    /**
     * @brief Gets the frequency identifier of the currently active wavelength.
     * Delegates the call to WavelengthStateManager.
     * @return The active frequency string, or "-1" if none.
     */
    static QString GetActiveWavelength();

    /**
     * @brief Sets the specified frequency as the currently active wavelength.
     * Delegates the call to WavelengthStateManager.
     * @param frequency The frequency identifier to set as active.
     */
    static void SetActiveWavelength(const QString &frequency);

    /**
     * @brief Checks if the current user is the host of the currently active wavelength.
     * Delegates the call to WavelengthStateManager.
     * @return True if the user is the host of the active wavelength, false otherwise.
     */
    static bool IsActiveWavelengthHost();

    /**
     * @brief Gets a list of frequencies the user is currently joined to.
     * Delegates the call to WavelengthStateManager.
     * @return A QList<QString> of joined frequencies.
     */
    static QList<QString> GetJoinedWavelengths();

    /**
     * @brief Gets the count of wavelengths the user is currently joined to.
     * Delegates the call to WavelengthStateManager.
     * @return The number of joined wavelengths.
     */
    static int GetJoinedWavelengthCount();

    /**
     * @brief Checks if a specific wavelength is password protected.
     * Delegates the call to WavelengthStateManager.
     * @param frequency The frequency identifier.
     * @return True if password protected, false otherwise.
     */
    static bool IsWavelengthPasswordProtected(const QString &frequency);

    /**
     * @brief Checks if the current user is the host of a specific wavelength.
     * Delegates the call to WavelengthStateManager.
     * @param frequency The frequency identifier.
     * @return True if the user is the host, false otherwise.
     */
    static bool IsWavelengthHost(const QString &frequency);

    /**
     * @brief Checks if the user is considered joined to a specific wavelength (based on registry presence).
     * Delegates the call to WavelengthStateManager.
     * @param frequency The frequency identifier.
     * @return True if joined, false otherwise.
     */
    static bool IsWavelengthJoined(const QString &frequency);

    /**
     * @brief Checks if the WebSocket connection for a specific wavelength is established and valid.
     * Delegates the call to WavelengthStateManager.
     * @param frequency The frequency identifier.
     * @return True if connected, false otherwise.
     */
    static bool IsWavelengthConnected(const QString &frequency);

    /**
     * @brief Gets the configured relay server address.
     * Delegates the call to WavelengthConfig.
     * @return The server address string.
     */
    static QString GetRelayServerAddress();

    /**
     * @brief Sets the relay server address in the configuration.
     * Delegates the call to WavelengthConfig.
     * @param address The new server address string.
     */
    static void SetRelayServerAddress(const QString &address);

    /**
     * @brief Gets the full URL (ws://address:port) of the relay server.
     * Delegates the call to WavelengthConfig.
     * @return The full WebSocket URL string.
     */
    static QString GetRelayServerUrl();

signals:
    /** @brief Emitted when a wavelength is successfully created. Relayed from WavelengthCreator. */
    void wavelengthCreated(QString frequency);

    /** @brief Emitted when a wavelength is successfully joined. Relayed from WavelengthJoiner. */
    void wavelengthJoined(QString frequency);

    /** @brief Emitted when the user leaves the active wavelength. Relayed from WavelengthLeaver. */
    void wavelengthLeft(QString frequency);

    /** @brief Emitted when a wavelength is closed (by host or server). Relayed from multiple components. */
    void wavelengthClosed(QString frequency);

    /** @brief Emitted when a message is received. Relayed from WavelengthMessageProcessor/Joiner. */
    void messageReceived(QString frequency, const QString &message);

    /** @brief Emitted when a message is successfully sent. Relayed from WavelengthMessageService. */
    void messageSent(QString frequency, const QString &message);

    /** @brief Emitted when a connection error occurs. Relayed from WavelengthCreator/Joiner. */
    void connectionError(const QString &error_message);

    /** @brief Emitted when authentication fails during join. Relayed from WavelengthJoiner. */
    void authenticationFailed(QString frequency);

    /** @brief Emitted when the current user is kicked from a frequency. Relayed from WavelengthMessageProcessor. */
    void userKicked(QString frequency, const QString &reason);

    /** @brief Emitted when the active wavelength changes. Relayed from WavelengthStateManager. */
    void activeWavelengthChanged(QString frequency);

    /** @brief Emitted when PTT transmission is granted. Relayed from WavelengthMessageService. */
    void pttGranted(QString frequency);

    /** @brief Emitted when PTT transmission is denied. Relayed from WavelengthMessageService. */
    void pttDenied(QString frequency, QString reason);

    /** @brief Emitted when another user starts PTT transmission. Relayed from WavelengthMessageService. */
    void pttStartReceiving(QString frequency, QString sender_id);

    /** @brief Emitted when another user stops PTT transmission. Relayed from WavelengthMessageService. */
    void pttStopReceiving(QString frequency);

    /** @brief Emitted when raw audio data is received. Relayed from WavelengthMessageService. */
    void audioDataReceived(QString frequency, const QByteArray &audio_data);

    /** @brief Emitted when remote audio amplitude updates are received. Relayed from WavelengthMessageService. */
    void remoteAudioAmplitudeUpdate(QString frequency, qreal amplitude);

private slots:
    /**
     * @brief Slot triggered when a wavelength is created.
     * Relays the signal and publishes the event via WavelengthEventBroker.
     * @param frequency The frequency created.
     */
    void onWavelengthCreated(const QString &frequency);

    /**
     * @brief Slot triggered when a wavelength is joined.
     * Relays the signal and publishes the event via WavelengthEventBroker.
     * @param frequency The frequency joined.
     */
    void onWavelengthJoined(const QString &frequency);

    /**
     * @brief Slot triggered when the active wavelength is left.
     * Relays the signal and publishes the event via WavelengthEventBroker.
     * @param frequency The frequency left.
     */
    void onWavelengthLeft(const QString &frequency);

    /**
     * @brief Slot triggered when a wavelength is closed.
     * Relays the signal and publishes the event via WavelengthEventBroker.
     * @param frequency The frequency closed.
     */
    void onWavelengthClosed(const QString &frequency);

    /**
     * @brief Slot triggered when a message is received.
     * Relays the signal and publishes the event via WavelengthEventBroker.
     * @param frequency The frequency the message belongs to.
     * @param message The formatted message content.
     */
    void onMessageReceived(const QString &frequency, const QString &message);

    /**
     * @brief Slot triggered when a message is sent.
     * Relays the signal and publishes the event via WavelengthEventBroker.
     * @param frequency The frequency the message was sent to.
     * @param message The formatted message content.
     */
    void onMessageSent(const QString &frequency, const QString &message);

    /**
     * @brief Slot triggered when a connection error occurs.
     * Relays the signal and publishes the event via WavelengthEventBroker.
     * @param errorMessage Description of the error.
     */
    void onConnectionError(const QString &errorMessage);

    /**
     * @brief Slot triggered when authentication fails during join.
     * Relays the signal and publishes the event via WavelengthEventBroker.
     * @param frequency The frequency for which authentication failed.
     */
    void onAuthenticationFailed(const QString &frequency);

    /**
     * @brief Slot triggered when the active wavelength changes.
     * Relays the signal and publishes the event via WavelengthEventBroker.
     * @param frequency The new active frequency.
     */
    void onActiveWavelengthChanged(const QString &frequency);

    /**
     * @brief Slot triggered when a configuration setting changes.
     * Logs the change.
     * @param key The configuration key that changed.
     */
    static void onConfigChanged(const QString &key) {
        qDebug() << "Configuration changed:" << key;
    }

    /** @brief Relays the pttGranted signal. */
    void onPttGranted(const QString &frequency) {
        emit pttGranted(frequency);
    }

    /** @brief Relays the pttDenied signal. */
    void onPttDenied(const QString &frequency, const QString &reason) {
        emit pttDenied(frequency, reason);
    }

    /** @brief Relays the pttStartReceiving signal. */
    void onPttStartReceiving(const QString &frequency, const QString &sender_id) {
        emit pttStartReceiving(frequency, sender_id);
    }

    /** @brief Relays the pttStopReceiving signal. */
    void onPttStopReceiving(const QString &frequency) {
        emit pttStopReceiving(frequency);
    }

    /** @brief Relays the audioDataReceived signal. */
    void onAudioDataReceived(const QString &frequency, const QByteArray &audio_data) {
        emit audioDataReceived(frequency, audio_data);
    }

    /** @brief Relays the remoteAudioAmplitudeUpdate signal. */
    void onRemoteAudioAmplitudeUpdate(const QString &frequency, const qreal amplitude) {
        emit remoteAudioAmplitudeUpdate(frequency, amplitude);
    }

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional parent QObject.
     */
    explicit SessionCoordinator(QObject *parent = nullptr) : QObject(parent) {
    }

    /**
     * @brief Private destructor.
     */
    ~SessionCoordinator() override = default;

    /**
     * @brief Connects signals between the various Wavelength components.
     * Called during Initialize().
     */
    void ConnectSignals();

    /**
     * @brief Loads the application configuration using WavelengthConfig.
     * Sets default values if the configuration file doesn't exist. Called during Initialize().
     */
    static void LoadConfig();
};

#endif // WAVELENGTH_SESSION_COORDINATOR_H
