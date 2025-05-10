#ifndef WAVELENGTH_STREAM_DISPLAY_H
#define WAVELENGTH_STREAM_DISPLAY_H

#include "../../ui/chat/communication_stream.h"

/**
 * @brief A widget that manages and displays messages within a CommunicationStream.
 *
 * This class acts as a controller for the CommunicationStream widget. It receives messages
 * via the AddMessage slot, queues them, and processes them sequentially with a delay
 * to create a staggered display effect. It handles updating existing progress messages
 * based on their unique ID and manages the lifecycle of displayed messages. It also
 * provides slots to forward control signals (like audio amplitude and transmitting user)
 * to the underlying CommunicationStream.
 */
class StreamDisplay final : public QWidget {
    Q_OBJECT

    /**
     * @brief Internal struct to hold data for messages waiting in the queue.
     */
    struct MessageData {
        /** @brief The raw message content (potentially HTML). */
        QString content;
        /** @brief The sender identifier ("You", "SYSTEM", or extracted name). */
        QString sender;
        /** @brief Unique identifier for the message (used for progress updates). Empty for regular messages. */
        QString id;
        /** @brief The type of the message (Received, Transmitted, System). */
        StreamMessage::MessageType type;
        /** @brief Flag indicating if the content likely contains an attachment placeholder. */
        bool has_attachment = false;
    };

public:
    /**
     * @brief Constructs a WavelengthStreamDisplay widget.
     * Creates the main layout, instantiates the CommunicationStream, initializes the message queue,
     * progress message map, and the message processing timer.
     * @param parent Optional parent widget.
     */
    explicit StreamDisplay(QWidget *parent = nullptr);

    /**
     * @brief Sets the frequency identifier and optional name for the stream.
     * Updates the name displayed in the CommunicationStream and clears any existing messages.
     * @param frequency The frequency identifier (e.g., "100.0").
     * @param name Optional display name for the frequency.
     */
    void SetFrequency(const QString &frequency, const QString &name = QString());

    /**
     * @brief Adds a new message to the display queue or updates an existing progress message.
     * If the message_id is provided and corresponds to a currently displayed progress message,
     * the content of that message is updated. Otherwise, a new MessageData entry is created
     * (extracting sender, checking for attachments) and added to the processing queue.
     * Starts the processing timer if the queue was empty.
     * @param message The message content (potentially HTML).
     * @param message_id A unique identifier for progress messages, empty otherwise.
     * @param type The type of the message (Received, Transmitted, System).
     */
    void AddMessage(const QString &message, const QString &message_id, StreamMessage::MessageType type);


    /**
     * @brief Clears all messages from the display and the processing queue.
     * Calls ClearMessages() on the CommunicationStream, clears the internal queue and map,
     * and stops the message processing timer.
     */
    void Clear();

public slots:
    /**
     * @brief Sets the intensity of the glitch effect in the CommunicationStream.
     * @param intensity The desired glitch intensity level.
     */
    void SetGlitchIntensity(const qreal intensity) const {
        communication_stream_->SetGlitchIntensity(intensity);
    }

    /**
     * @brief Forwards the transmitting user ID to the CommunicationStream.
     * @param userId The identifier of the user currently transmitting audio.
     */
    void SetTransmittingUser(const QString &userId) const {
        communication_stream_->SetTransmittingUser(userId);
    }

    /**
     * @brief Tells the CommunicationStream to clear the transmitting user display.
     */
    void ClearTransmittingUser() const {
        communication_stream_->ClearTransmittingUser();
    }

    /**
     * @brief Forwards the audio amplitude level to the CommunicationStream.
     * @param amplitude The current audio amplitude level.
     */
    void SetAudioAmplitude(const qreal amplitude) const {
        communication_stream_->SetAudioAmplitude(amplitude);
    }

private slots:
    /**
     * @brief Processes the next message from the message_queue_.
     * Dequeues a message, checks if it's an update to an existing progress message.
     * If not, calls the appropriate AddMessage method on CommunicationStream.
     * If it's a progress message, adds the returned StreamMessage pointer to the
     * displayed_progress_messages_ map and connects its destroyed() signal.
     * Restarts the timer with a random delay if the queue is not empty.
     */
    void ProcessNextQueuedMessage();

    /**
     * @brief Slot called when a StreamMessage associated with a progress ID is destroyed.
     * Removes the corresponding entry from the displayed_progress_messages_ map to prevent
     * dangling pointers and incorrect update attempts.
     * @param object Pointer to the QObject that was destroyed (the StreamMessage).
     */
    void OnStreamMessageDestroyed(const QObject *object);

private:
    /** @brief The underlying widget that handles the visual rendering of the stream and messages. */
    CommunicationStream *communication_stream_;
    /** @brief Queue holding messages waiting to be processed and displayed. */
    QQueue<MessageData> message_queue_;
    /** @brief Timer controlling the delay between displaying queued messages. */
    QTimer *message_timer_;
    /** @brief Map tracking currently displayed progress messages by their unique ID to allow updates. */
    QMap<QString, StreamMessage *> displayed_progress_messages_;
};

#endif // WAVELENGTH_STREAM_DISPLAY_H
