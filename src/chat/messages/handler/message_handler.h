#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <QJsonObject>
#include <QObject>
#include <QUuid>

class QWebSocket;

/**
 * @brief Singleton class responsible for handling WebSocket message operations.
 *
 * Provides static methods for creating, parsing, and extracting information from
 * JSON-based messages used in the WebSocket communication protocol. It also manages
 * to send system commands and keeps track of processed message IDs to prevent duplicates.
 */
class MessageHandler final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the MessageHandler.
     * @return Pointer to the singleton MessageHandler instance.
     */
    static MessageHandler *GetInstance() {
        static MessageHandler instance;
        return &instance;
    }

    /**
     * @brief Generates a unique message identifier using UUID.
     * @return A unique QString identifier (UUID without braces).
     */
    static QString GenerateMessageId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    /**
     * @brief Sends a JSON-formatted system command over the specified WebSocket.
     * Constructs a JSON object with the given command type and parameters,
     * converts it to a JSON string, and sends it as a text message.
     * @param socket Pointer to the QWebSocket to send the command through.
     * @param command The type string for the command (e.g., "auth", "register_wavelength").
     * @param params Optional QJsonObject containing additional parameters for the command.
     * @return True if the command was sent successfully (socket valid), false otherwise.
     */
    static bool SendSystemCommand(QWebSocket *socket, const QString &command,
                                  const QJsonObject &params = QJsonObject());

    /**
     * @brief Checks if a message with the given ID has already been processed.
     * @param message_id The unique identifier of the message to check.
     * @return True if the message ID exists in the processed set, false otherwise.
     */
    bool IsMessageProcessed(const QString &message_id) const {
        return !message_id.isEmpty() && processed_message_ids_.contains(message_id);
    }

    /**
     * @brief Marks a message ID as processed by adding it to the internal set.
     * Also manages the size of the processed ID cache, removing older entries if the maximum size is exceeded.
     * @param message_id The unique identifier of the message to mark as processed.
     */
    void MarkMessageAsProcessed(const QString &message_id);

    /**
     * @brief Parses a JSON string message into a QJsonObject.
     * @param message The QString containing the JSON message data.
     * @param ok Optional pointer to a boolean flag that will be set to true on success, false on failure.
     * @return The parsed QJsonObject on success, or an empty QJsonObject on failure.
     */
    static QJsonObject ParseMessage(const QString &message, bool *ok = nullptr);

    /**
     * @brief Extracts the message type string from a parsed message object.
     * @param message_object The QJsonObject representing the parsed message.
     * @return The value of the "type" field as a QString.
     */
    static QString GetMessageType(const QJsonObject &message_object) {
        return message_object["type"].toString();
    }

    /**
     * @brief Extracts the message content string from a parsed message object.
     * @param message_object The QJsonObject representing the parsed message.
     * @return The value of the "content" field as a QString.
     */
    static QString GetMessageContent(const QJsonObject &message_object) {
        return message_object["content"].toString();
    }

    /**
     * @brief Extracts the frequency string from a parsed message object.
     * @param message_object The QJsonObject representing the parsed message.
     * @return The value of the "frequency" field as a QString.
     */
    static QString GetMessageFrequency(const QJsonObject &message_object) {
        return message_object["frequency"].toString();
    }

    /**
     * @brief Extracts the sender ID string from a parsed message object.
     * @param message_object The QJsonObject representing the parsed message.
     * @return The value of the "senderId" field as a QString.
     */
    static QString GetMessageSenderId(const QJsonObject &message_object) {
        return message_object["senderId"].toString();
    }

    /**
     * @brief Extracts the message ID string from a parsed message object.
     * @param message_object The QJsonObject representing the parsed message.
     * @return The value of the "messageId" field as a QString.
     */
    static QString GetMessageId(const QJsonObject &message_object) {
        return message_object["messageId"].toString();
    }

    /**
     * @brief Creates a JSON object representing an authentication request.
     * @param frequency The frequency the client wants to join.
     * @param password The password for the frequency (if required).
     * @param client_id The unique identifier of the client sending the request.
     * @return A QJsonObject formatted as an authentication request.
     */
    static QJsonObject CreateAuthRequest(const QString &frequency, const QString &password, const QString &client_id);

    /**
     * @brief Creates a JSON object representing a request to register a new frequency.
     * @param frequency The desired name for the new frequency.
     * @param is_password_protected Flag indicating if the frequency should require a password.
     * @param password The password to set for the frequency (if protected).
     * @param host_id The unique identifier of the client hosting the frequency.
     * @return A QJsonObject formatted as a frequency registration request.
     */
    static QJsonObject CreateRegisterRequest(const QString &frequency, bool is_password_protected,
                                             const QString &password, const QString &host_id);

    /**
     * @brief Creates a JSON object representing a request to leave or close a frequency.
     * @param frequency The frequency to leave or close.
     * @param is_host True if the client is the host (closing the frequency), false otherwise (leaving).
     * @return A QJsonObject formatted as a leave or close request.
     */
    static QJsonObject CreateLeaveRequest(const QString &frequency, bool is_host);

    /**
     * @brief Clears the internal cache of processed message IDs.
     */
    void ClearProcessedMessages() {
        processed_message_ids_.clear();
    }

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional parent QObject.
     */
    explicit MessageHandler(QObject *parent = nullptr) : QObject(parent) {
    }

    /**
     * @brief Private destructor.
     */
    ~MessageHandler() override = default;

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    MessageHandler(const MessageHandler &) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    MessageHandler &operator=(const MessageHandler &) = delete;

    /** @brief Set storing the IDs of messages that have already been processed to prevent duplicates. */
    QSet<QString> processed_message_ids_;
    /** @brief Maximum number of message IDs to keep in the processed cache before removing older ones. */
    static constexpr int kMaxCachedMessageIds = 200;
};

#endif // MESSAGE_HANDLER_H
