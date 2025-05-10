#include "wavelength_stream_display.h"

#include <QRegularExpression>

WavelengthStreamDisplay::WavelengthStreamDisplay(QWidget *parent): QWidget(parent) {
    const auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);

    communication_stream_ = new CommunicationStream(this);
    communication_stream_->setMinimumHeight(300);
    main_layout->addWidget(communication_stream_, 1);

    message_queue_ = QQueue<MessageData>();
    displayed_progress_messages_ = QMap<QString, StreamMessage *>();

    message_timer_ = new QTimer(this);
    message_timer_->setSingleShot(true);
    connect(message_timer_, &QTimer::timeout, this, &WavelengthStreamDisplay::ProcessNextQueuedMessage);
}

void WavelengthStreamDisplay::SetFrequency(const QString &frequency, const QString &name) {
    if (name.isEmpty()) {
        communication_stream_->SetStreamName(QString("WAVELENGTH: %1 Hz").arg(frequency));
    } else {
        communication_stream_->SetStreamName(QString("%1 (%2 Hz)").arg(name, frequency));
    }
    Clear();
}

void WavelengthStreamDisplay::AddMessage(const QString &message, const QString &message_id,
                                         const StreamMessage::MessageType type) {
    if (!message_id.isEmpty() && displayed_progress_messages_.contains(message_id)) {
        if (StreamMessage *existing_message = displayed_progress_messages_.value(message_id)) {
            existing_message->UpdateContent(message);
            return;
        }
        qWarning() << "[STREAM DISPLAY] Removing an invalid indicator from the map for ID:" << message_id;
        displayed_progress_messages_.remove(message_id);
    }

    MessageData data;
    data.content = message;
    data.id = message_id;
    data.type = type;
    data.has_attachment = message.contains("image-placeholder") ||
                          message.contains("video-placeholder") ||
                          message.contains("audio-placeholder") ||
                          message.contains("gif-placeholder");

    if (type == StreamMessage::kTransmitted && message_id.isEmpty()) {
        data.sender = "You";
    } else if (type == StreamMessage::kReceived) {
        const QRegularExpression regular_expression("<span[^>]*>([^<]+):</span>");
        const QRegularExpressionMatch match = regular_expression.match(message);
        if (match.hasMatch()) {
            data.sender = match.captured(1);
        } else {
            data.sender = "Unknown";
        }
    } else {
        data.sender = "SYSTEM";
    }

    message_queue_.enqueue(data);

    if (message_queue_.size() == 1 && !message_timer_->isActive()) {
        message_timer_->start(100);
    }
}

void WavelengthStreamDisplay::Clear() {
    communication_stream_->ClearMessages();
    message_queue_.clear();
    displayed_progress_messages_.clear();
    message_timer_->stop();
}

void WavelengthStreamDisplay::ProcessNextQueuedMessage() {
    if (message_queue_.isEmpty()) {
        return;
    }
    const auto [content, sender, id, type, has_attachment] = message_queue_.dequeue();
    StreamMessage *displayed_message = nullptr;

    if (!id.isEmpty() && displayed_progress_messages_.contains(id)) {
        qWarning() << "[STREAM DISPLAY] Trying to add a message with an ID that already exists in the map:" << id;
        displayed_message = displayed_progress_messages_.value(id);
        if (displayed_message) {
            displayed_message->UpdateContent(content);
        } else {
            displayed_progress_messages_.remove(id);
            displayed_message = nullptr;
        }
    }

    if (!displayed_message) {
        if (has_attachment) {
            displayed_message = communication_stream_->AddMessageWithAttachment(
                content, sender, type, id);
        } else {
            displayed_message = communication_stream_->AddMessage(
                content, sender, type, id);
        }
    }

    if (displayed_message) {
        if (!id.isEmpty()) {
            if (!displayed_progress_messages_.contains(id)) {
                displayed_progress_messages_.insert(id, displayed_message);
                connect(displayed_message, &QObject::destroyed, this,
                        &WavelengthStreamDisplay::OnStreamMessageDestroyed);
            } else {
                qWarning() << "[STREAM DISPLAY] Progress message with ID " << id << "already exists in the map!";
            }
        }
    } else {
        qWarning() << "[STREAM DISPLAY] CommunicationStream failed to create/return message widget for ID:" << id;
    }

    if (!message_queue_.isEmpty()) {
        const int delay = 300 + QRandomGenerator::global()->bounded(500);
        message_timer_->start(delay);
    }
}

void WavelengthStreamDisplay::OnStreamMessageDestroyed(const QObject *object) {
    auto it = displayed_progress_messages_.begin();
    while (it != displayed_progress_messages_.end()) {
        if (it.value() == object) {
            displayed_progress_messages_.erase(it);
            return;
        }
        ++it;
    }
    qWarning() <<
            "[STREAM DISPLAY] Destroyed signal has been received for an object that is not in the progress message map.";
}
