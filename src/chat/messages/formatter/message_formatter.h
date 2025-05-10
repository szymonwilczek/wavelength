#ifndef MESSAGE_FORMATTER_H
#define MESSAGE_FORMATTER_H

#include "../../../storage/wavelength_registry.h"
#include "../../files/attachments/attachment_data_store.h"

/**
 * @brief Provides static utility functions for formatting messages into HTML strings.
 *
 * This class handles the conversion of message data (from JSON objects or simple strings)
 * into HTML suitable for display in the chat interface. It manages timestamps, sender
 * identification, text formatting, and attachment handling (using placeholders).
 */
class MessageFormatter {
public:
    /**
     * @brief Formats a message represented by a JSON object into an HTML string.
     *
     * Extracts sender, content, timestamp, and attachment information from the JSON.
     * Determines if the message is from the current user ("You").
     * For attachments, it stores the base64 data (if present and not already stored)
     * in the AttachmentDataStore and generates an HTML placeholder div element.
     * This placeholder contains `data-*` attributes (like `data-attachment-id`, `data-mime-type`,
     * `data-filename`) that reference the stored data, keeping the HTML itself lightweight.
     * For text messages, it formats the content with appropriate sender info and timestamp.
     *
     * @param message_object The JSON object containing the message details. Expected keys include
     *                       "content", "sender"/"senderId", "timestamp", "isSelf", "hasAttachment",
     *                       "attachmentType", "attachmentName", "attachmentMimeType", "attachmentData".
     * @param frequency The frequency identifier, used to look up sender names via WavelengthRegistry if only senderId is present.
     * @return An HTML formatted QString representing the message.
     */
    static QString FormatMessage(const QJsonObject &message_object, const QString &frequency);

    /**
     * @brief Formats a file size in bytes into a human-readable string (e.g., "1.2 MB").
     *
     * Converts the byte count into KB, MB, or GB with appropriate precision.
     *
     * @param size_in_bytes The file size in bytes.
     * @return A QString representing the formatted file size.
     */
    static QString FormatFileSize(qint64 size_in_bytes);

    /**
     * @brief Formats a simple string as a system message.
     *
     * Prepends the current timestamp and applies specific styling (yellow color) to the message text.
     *
     * @param message The text content of the system message.
     * @return An HTML formatted QString representing the system message.
     */
    static QString FormatSystemMessage(const QString &message);
};

#endif // MESSAGE_FORMATTER_H
