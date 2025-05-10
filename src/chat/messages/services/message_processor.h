#ifndef WAVELENGTH_MESSAGE_PROCESSOR_H
#define WAVELENGTH_MESSAGE_PROCESSOR_H

#include "message_service.h"
#include "../handler/message_handler.h"

class MessageService;

/**
 * @brief Singleton class responsible for processing incoming WebSocket messages for specific wavelengths.
 *
 * This class acts as the central hub for interpreting messages received from the server
 * for a given frequency (wavelength). It parses JSON messages, identifies their type,
 * checks for duplicates, validates frequency matching, and dispatches them to appropriate
 * private handler methods. It also handles incoming binary data (audio).
 * It emits signals based on the processed message content (e.g., new message, user joined/left,
 * PTT events, audio data). It collaborates with MessageHandler for parsing and ID management,
 * MessageFormatter for creating displayable HTML, and AttachmentDataStore for handling attachments.
 */
class MessageProcessor final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the WavelengthMessageProcessor.
     * @return Pointer to the singleton WavelengthMessageProcessor instance.
     */
    static MessageProcessor *GetInstance() {
        static MessageProcessor instance;
        return &instance;
    }

    /**
     * @brief Compares two frequency strings for equality.
     * Simple string comparison.
     * @param frequency1 The first frequency string.
     * @param frequency2 The second frequency string.
     * @return True if the strings are identical, false otherwise.
     */
    static bool AreFrequenciesEqual(const QString &frequency1, const QString &frequency2) {
        return frequency1 == frequency2;
    }

    /**
     * @brief Processes an incoming text message (JSON) received from the WebSocket.
     * Parses the JSON, checks message ID, validates frequency, determines a message type,
     * and calls the corresponding private processing method (e.g., ProcessMessageContent, ProcessSystemCommand).
     * @param message The raw JSON message string.
     * @param frequency Frequency/wavelength this message belongs to.
     */
    void ProcessIncomingMessage(const QString &message, const QString &frequency);

    /**
     * @brief Processes an incoming binary message (expected to be audio data) received from the WebSocket.
     * Emits the audioDataReceived signal with the raw audio data.
     * @param message The raw binary data (QByteArray).
     * @param frequency The frequency/wavelength this data belongs to.
     */
    void ProcessIncomingBinaryMessage(const QByteArray &message, const QString &frequency);

    /**
     * @brief Connects the appropriate slots of this processor to the signals of a given QWebSocket.
     * Disconnects any previous handlers for the socket first. Connects textMessageReceived
     * and binaryMessageReceived signals to the respective processing methods of this class,
     * capturing the associated frequency. Also connects the socket's error signal for logging.
     * @param socket The QWebSocket instance to connect handlers to.
     * @param frequency The frequency associated with this socket connection.
     */
    void SetSocketMessageHandlers(QWebSocket *socket, QString frequency);

private:
    /**
     * @brief Processes messages of type "message" or "send_message".
     * Checks for duplicates, handles attachments (storing data and creating placeholders if necessary),
     * formats the message using MessageFormatter, and emits the messageReceived signal.
     * @param message_object The parsed JSON object of the message.
     * @param frequency The frequency the message belongs to.
     * @param message_id The unique ID of the message.
     */
    void ProcessMessageContent(const QJsonObject &message_object, const QString &frequency, const QString &message_id);

    /**
     * @brief Processes messages of type "system_command".
     * Handles commands like "ping", "close_wavelength", "kick_user".
     * Emits appropriate signals (e.g., userKicked, wavelengthClosed).
     * @param message_object The parsed JSON object of the command.
     * @param frequency The frequency the command applies to.
     */
    void ProcessSystemCommand(const QJsonObject &message_object, const QString &frequency);

    /**
     * @brief Processes messages of type "user_joined".
     * Formats a system message indicating a user joined and emits the systemMessage signal.
     * @param message_object The parsed JSON object of the event.
     * @param frequency The frequency the user joined.
     */
    void ProcessUserJoined(const QJsonObject &message_object, const QString &frequency);

    /**
     * @brief Processes messages of type "user_left".
     * Formats a system message indicating a user left and emits the systemMessage signal.
     * @param message_object The parsed JSON object of the event.
     * @param frequency The frequency the user left.
     */
    void ProcessUserLeft(const QJsonObject &message_object, const QString &frequency);

    /**
     * @brief Processes messages indicating a wavelength was closed (e.g., "wavelength_closed", "close_wavelength" command).
     * Updates the WavelengthRegistry and emits the wavelengthClosed signal.
     * @param frequency The frequency that was closed.
     */
    void ProcessWavelengthClosed(const QString &frequency);

signals:
    /**
     * @brief Emitted when a regular chat message (text or attachment placeholder) is processed.
     * @param frequency The frequency the message belongs to.
     * @param formatted_message The HTML-formatted message string for display.
     */
    void messageReceived(QString frequency, const QString &formatted_message);

    /**
     * @brief Emitted when a system event message (e.g., user join/leave) is processed.
     * @param frequency The frequency the event occurred on.
     * @param formatted_message The HTML-formatted system message string.
     */
    void systemMessage(QString frequency, const QString &formatted_message);

    /**
     * @brief Emitted when a wavelength is closed (either by host or server command).
     * @param frequency The frequency that was closed.
     */
    void wavelengthClosed(QString frequency);

    /**
     * @brief Emitted when the current user is kicked from a frequency.
     * @param frequency The frequency the user was kicked from.
     * @param reason The reason provided for the kick.
     */
    void userKicked(QString frequency, const QString &reason);

    /**
     * @brief Emitted when the server grants permission to transmit audio (Push-to-Talk).
     * @param frequency The frequency for which PTT was granted.
     */
    void pttGranted(QString frequency);

    /**
     * @brief Emitted when the server denies permission to transmit audio (Push-to-Talk).
     * @param frequency The frequency for which PTT was denied.
     * @param reason The reason provided for the denial.
     */
    void pttDenied(QString frequency, QString reason);

    /**
     * @brief Emitted when another user starts transmitting audio on the frequency.
     * @param frequency The frequency where transmission started.
     * @param sender_id The ID of the user transmitting.
     */
    void pttStartReceiving(QString frequency, QString sender_id);

    /**
     * @brief Emitted when the currently transmitting user stops sending audio.
     * @param frequency The frequency where transmission stopped.
     */
    void pttStopReceiving(QString frequency);

    /**
     * @brief Emitted when raw audio data is received via a binary WebSocket message.
     * @param frequency The frequency the audio data belongs to.
     * @param audio_data The raw audio data bytes.
     */
    void audioDataReceived(QString frequency, const QByteArray &audio_data);

    /**
     * @brief Emitted when the server sends an update about the remote audio amplitude (optional).
     * @param frequency The frequency the amplitude update is for.
     * @param amplitude The current amplitude level (typically 0.0 to 1.0).
     */
    void remoteAudioAmplitudeUpdate(QString frequency, qreal amplitude);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * Connects internal signals to the corresponding slots in WavelengthMessageService.
     * @param parent Optional parent QObject.
     */
    explicit MessageProcessor(QObject *parent = nullptr);

    /**
     * @brief Private destructor.
     */
    ~MessageProcessor() override {
    }

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    MessageProcessor(const MessageProcessor &) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    MessageProcessor &operator=(const MessageProcessor &) = delete;
};

#endif // WAVELENGTH_MESSAGE_PROCESSOR_H
