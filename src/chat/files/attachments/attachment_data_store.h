#ifndef ATTACHMENT_DATA_STORE_H
#define ATTACHMENT_DATA_STORE_H

#include <QMap>
#include <QMutex> // Corrected include from qmutex.h
#include <QString>

/**
 * @brief Manages temporary storage of attachment data using a singleton pattern.
 *
 * This class provides a thread-safe, in-memory store for holding base64-encoded
 * attachment data associated with unique IDs. It's used to temporarily hold
 * attachment content before it's fully processed or sent.
 */
class AttachmentDataStore {
public:
    /**
     * @brief Gets the singleton instance of the AttachmentDataStore.
     * @return Pointer to the singleton AttachmentDataStore instance.
     */
    static AttachmentDataStore* GetInstance() {
        static AttachmentDataStore instance;
        return &instance;
    }

    /**
     * @brief Stores base64-encoded attachment data and returns a unique ID.
     * Generates a UUID for the attachment and stores the data in the internal map.
     * This operation is thread-safe.
     * @param base64_data The attachment data encoded as a base64 QString.
     * @return A unique QString identifier (UUID without braces) for the stored data.
     */
    QString StoreAttachmentData(const QString& base64_data);

    /**
     * @brief Retrieves the base64-encoded attachment data associated with a given ID.
     * This operation is thread-safe.
     * @param attachment_id The unique identifier of the attachment data to retrieve.
     * @return The base64-encoded QString data if the ID exists, otherwise an empty QString.
     */
    QString GetAttachmentData(const QString& attachment_id);

    /**
     * @brief Removes the attachment data associated with the given ID from the store.
     * Used to free up memory once the attachment data is no longer needed.
     * This operation is thread-safe.
     * @param attachment_id The unique identifier of the attachment data to remove.
     */
    void RemoveAttachmentData(const QString& attachment_id);

private:
    /**
     * @brief Private default constructor to enforce the singleton pattern.
     */
    AttachmentDataStore() = default;

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    AttachmentDataStore(const AttachmentDataStore&) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    AttachmentDataStore& operator=(const AttachmentDataStore&) = delete;

    /**
     * @brief Map storing the attachment data.
     * Key: Attachment ID (QString UUID).
     * Value: Base64-encoded attachment data (QString).
     */
    QMap<QString, QString> attachment_data_;

    /**
     * @brief Mutex ensuring thread-safe access to the attachment_data_ map.
     */
    QMutex mutex_;
};

#endif //ATTACHMENT_DATA_STORE_H