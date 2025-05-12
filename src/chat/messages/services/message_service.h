#ifndef WAVELENGTH_MESSAGE_SERVICE_H
#define WAVELENGTH_MESSAGE_SERVICE_H

#include <QMap>
#include <QObject>

class QWebSocket;

/**
 * @brief Singleton service responsible for sending messages and files over WebSocket connections.
 *
 * This class manages the sending of text messages, files (as base64 encoded JSON),
 * Push-to-Talk (PTT) requests/releases, and raw audio data for specific frequencies (wavelengths).
 * It interacts with WavelengthRegistry to find the appropriate WebSocket connection for a given frequency.
 * File sending is handled asynchronously using AttachmentQueueManager to avoid blocking the main thread.
 * It provides signals for tracking message sending progress and status, as well as PTT and audio events.
 */
class MessageService final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the MessageService.
     * @return Pointer to the singleton MessageService instance.
     */
    static MessageService *GetInstance() {
        static MessageService instance;
        return &instance;
    }

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    MessageService(const MessageService &) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    MessageService &operator=(const MessageService &) = delete;

    /**
     * @brief Sends a Push-to-Talk (PTT) request message for the specified frequency.
     * Constructs a JSON message of type "request_ptt" and sends it via the frequency's socket.
     * @param frequency The frequency for which PTT is requested.
     * @return True if the request was sent successfully (socket valid), false otherwise.
     */
    static bool SendPttRequest(const QString &frequency);

    /**
     * @brief Sends a Push-to-Talk (PTT) release message for the specified frequency.
     * Constructs a JSON message of type "release_ptt" and sends it via the frequency's socket.
     * @param frequency The frequency for which PTT is being released.
     * @return True if the release message was sent successfully (socket valid), false otherwise.
     */
    static bool SendPttRelease(const QString &frequency);

    /**
     * @brief Sends raw audio data as a binary message for the specified frequency.
     * Used for transmitting audio during an active PTT session.
     * @param frequency The frequency the audio data belongs to.
     * @param audio_data The raw audio data bytes.
     * @return True if the binary message was sent successfully (socket valid and data sent), false otherwise.
     */
    static bool SendAudioData(const QString &frequency, const QByteArray &audio_data);

    /**
     * @brief Sends a text message to the currently active frequency.
     * Retrieves the active frequency and socket from WavelengthRegistry.
     * Constructs a JSON message of type "send_message" including content, sender ID, timestamp,
     * and a unique message ID. Sends the message via the socket. Caches sent message content locally.
     * @param message The text content of the message to send.
     * @return True if the message was sent successfully, false if no active frequency, socket invalid, etc.
     */
    bool SendTextMessage(const QString &message);

    /**
     * @brief Initiates the process of sending a file to the currently active frequency.
     * Reads the file, encodes it to base64, determines a file type, and constructs a JSON message
     * of a type "send_file" containing the metadata and encoded data.
     * This entire process (reading, encoding) is offloaded to a background thread using AttachmentQueueManager.
     * Emits progressMessageUpdated signals to track the process. Finally, emits sendJsonViaSocket
     * to trigger the actual sending on the main thread.
     * @param file_path The local path to the file to be sent.
     * @param progress_message_id Optional unique ID to associate with progress update messages. If empty, a new one is generated.
     * @return True if the file sending a task was successfully queued, false otherwise (e.g., empty path).
     */
    bool SendFile(const QString &file_path, const QString &progress_message_id = QString());

    /**
     * @brief Sends a pre-formatted JSON message (typically containing file data) to the server.
     * This method is intended to be called internally or via signals after file processing.
     * Finds the socket for the given frequency and sends the JSON string.
     * Updates the progress message to indicate success or failure. Schedules removal of the progress message.
     * @param json_message The JSON string message to send.
     * @param frequency The target frequency.
     * @param progress_message_id The ID associated with the progress message for this file transfer.
     * @return True if the message was sent successfully, false if the socket is invalid.
     * @deprecated This method seems redundant with HandleSendJsonViaSocket and might be removed.
     */
    bool SendFileToServer(const QString &json_message, const QString &frequency, const QString &progress_message_id);


    /**
     * @brief Gets a pointer to the internal cache of sent message contents.
     * The cache maps message ID to message content.
     * @return Pointer to the QMap storing sent message contents.
     */
    QMap<QString, QString> *GetSentMessageCache() {
        return &sent_messages_;
    }

    /**
     * @brief Clears the internal cache of sent message contents.
     */
    void ClearSentMessageCache() {
        sent_messages_.clear();
    }

    /**
     * @brief Gets the client ID currently associated with this service instance.
     * @return The client ID string.
     */
    [[nodiscard]] QString GetClientId() const {
        return client_id_;
    }

    /**
     * @brief Sets the client ID for this service instance.
     * @param clientId The client ID string to set.
     */
    void SetClientId(const QString &clientId) {
        client_id_ = clientId;
    }

public slots:
    /**
     * @brief Slot to update a progress message associated with a file transfer.
     * Emits the progressMessageUpdated signal.
     * @param progress_message_id The ID of the progress message to update.
     * @param message The new HTML-formatted status message.
     */
    void UpdateProgressMessage(const QString &progress_message_id, const QString &message) {
        if (progress_message_id.isEmpty()) return;
        emit progressMessageUpdated(progress_message_id, message);
    }

    /**
     * @brief Slot connected to the sendJsonViaSocket signal. Performs the actual sending of a JSON message.
     * Finds the socket for the frequency and sends the message. Updates the progress message
     * to indicate success or failure. This runs on the main thread.
     * @param json_message The JSON string message to send (usually containing file data).
     * @param frequency The target frequency.
     * @param progress_message_id The ID associated with the progress message for this transfer.
     */
    void HandleSendJsonViaSocket(const QString &json_message, const QString &frequency,
                                 const QString &progress_message_id);

signals:
    /**
     * @brief Emitted immediately after a text message is successfully sent via the socket.
     * @param frequency The frequency the message was sent to.
     * @param formatted_message The HTML-formatted version of the sent message (for local display).
     */
    void messageSent(QString frequency, const QString &formatted_message);

    /**
     * @brief Emitted during file processing and sending to update the status display.
     * @param message_id The unique ID associated with this file transfer's progress messages.
     * @param message The HTML-formatted status update message.
     */
    void progressMessageUpdated(const QString &message_id, const QString &message);

    /**
     * @brief Emitted after a file transfer is complete (success or failure) to remove the progress message.
     * @param message_id The unique ID of the progress message to remove.
     */
    void removeProgressMessage(const QString &message_id);

    /**
     * @brief Internal signal emitted by the background file processing task when the JSON message is ready to be sent.
     * Connected to the HandleSendJsonViaSocket slot.
     * @param json_message The complete JSON message string containing file metadata and base64 data.
     * @param frequency The target frequency.
     * @param progress_message_id The ID associated with the progress message for this transfer.
     */
    void sendJsonViaSocket(const QString &json_message, QString frequency, const QString &progress_message_id);

    /**
     * @brief Emitted when the server grants permission to transmit audio (Push-to-Talk).
     * Relayed from WavelengthMessageProcessor.
     * @param frequency The frequency for which PTT was granted.
     */
    void pttGranted(QString frequency);

    /**
     * @brief Emitted when the server denies permission to transmit audio (Push-to-Talk).
     * Relayed from WavelengthMessageProcessor.
     * @param frequency The frequency for which PTT was denied.
     * @param reason The reason provided for the denial.
     */
    void pttDenied(QString frequency, QString reason);

    /**
     * @brief Emitted when another user starts transmitting audio on the frequency.
     * Relayed from WavelengthMessageProcessor.
     * @param frequency The frequency where transmission started.
     * @param sender_id The ID of the user transmitting.
     */
    void pttStartReceiving(QString frequency, QString sender_id);

    /**
     * @brief Emitted when the currently transmitting user stops sending audio.
     * Relayed from WavelengthMessageProcessor.
     * @param frequency The frequency where transmission stopped.
     */
    void pttStopReceiving(QString frequency);

    /**
     * @brief Emitted when raw audio data is received via a binary WebSocket message.
     * Relayed from WavelengthMessageProcessor.
     * @param frequency The frequency the audio data belongs to.
     * @param audio_data The raw audio data bytes.
     */
    void audioDataReceived(QString frequency, const QByteArray &audio_data);

    /**
     * @brief Emitted when the server sends an update about the remote audio amplitude (optional).
     * Relayed from WavelengthMessageProcessor.
     * @param frequency The frequency the amplitude update is for.
     * @param amplitude The current amplitude level (typically 0.0 to 1.0).
     */
    void remoteAudioAmplitudeUpdate(QString frequency, qreal amplitude);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * Connects the internal sendJsonViaSocket signal to the HandleSendJsonViaSocket slot.
     * @param parent Optional parent QObject.
     */
    explicit MessageService(QObject *parent = nullptr);

    /**
     * @brief Private destructor.
     */
    ~MessageService() override = default;

    /**
     * @brief Helper function to retrieve the active WebSocket connection for a given frequency.
     * Uses WavelengthRegistry to look up the socket. Includes checks for validity.
     * @param frequency The frequency whose socket is needed.
     * @return Pointer to the QWebSocket if found and valid, nullptr otherwise.
     */
    static QWebSocket *GetSocketForFrequency(const QString &frequency);

    /** @brief Cache storing the content of recently sent text messages, mapped by message ID. */
    QMap<QString, QString> sent_messages_;
    /** @brief Stores the client ID associated with this service instance. */
    QString client_id_;
};

#endif // WAVELENGTH_MESSAGE_SERVICE_H
