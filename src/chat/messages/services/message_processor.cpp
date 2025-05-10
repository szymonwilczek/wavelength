#include "message_processor.h"

#include "message_service.h"
#include "../../../storage/wavelength_registry.h"
#include "../../files/attachments/attachment_data_store.h"
#include "../formatter/message_formatter.h"
#include "../handler/message_handler.h"

void MessageProcessor::ProcessIncomingMessage(const QString &message, const QString &frequency) {
    bool ok = false;
    const QJsonObject message_object = MessageHandler::GetInstance()->ParseMessage(message, &ok);

    if (!ok) {
        qDebug() << "[MESSAGE PROCESSOR] Failed to parse JSON message.";
        return;
    }

    const QString message_type = MessageHandler::GetInstance()->GetMessageType(message_object);
    const QString message_id = MessageHandler::GetInstance()->GetMessageId(message_object);
    const QString message_frequency = MessageHandler::GetInstance()->GetMessageFrequency(message_object);

    if (!AreFrequenciesEqual(message_frequency, frequency) && message_frequency != -1) {
        qDebug() << "[MESSAGE PROCESSOR] Message frequency mismatch:" << message_frequency << "vs" << frequency;
        return;
    }

    if (MessageHandler::GetInstance()->IsMessageProcessed(message_id)) {
        qDebug() << "[MESSAGE PROCESSOR] Message already processed:" << message_id;
        return;
    }

    if (message_type == "message" || message_type == "send_message") {
        ProcessMessageContent(message_object, frequency, message_id);
    } else if (message_type == "system_command") {
        ProcessSystemCommand(message_object, frequency);
    } else if (message_type == "user_joined") {
        ProcessUserJoined(message_object, frequency);
    } else if (message_type == "user_left") {
        ProcessUserLeft(message_object, frequency);
    } else if (message_type == "wavelength_closed") {
        ProcessWavelengthClosed(frequency);
    } else if (message_type == "ptt_granted") {
        emit pttGranted(frequency);
    } else if (message_type == "ptt_denied") {
        const QString reason = message_object.value("reason").toString("Transmission slot is busy.");
        emit pttDenied(frequency, reason);
    } else if (message_type == "ptt_start_receiving") {
        const QString senderId = message_object.value("senderId").toString("Unknown");
        emit pttStartReceiving(frequency, senderId);
    } else if (message_type == "ptt_stop_receiving") {
        emit pttStopReceiving(frequency);
    } else if (message_type == "audio_amplitude") {
        const qreal amplitude = message_object.value("amplitude").toDouble(0.0);
        emit remoteAudioAmplitudeUpdate(frequency, amplitude);
    } else {
        qDebug() << "[MESSAGE PROCESSOR] Unknown message type received:" << message_type;
    }
}

void MessageProcessor::ProcessIncomingBinaryMessage(const QByteArray &message, const QString &frequency) {
    emit audioDataReceived(frequency, message);
}

void MessageProcessor::SetSocketMessageHandlers(QWebSocket *socket, QString frequency) {
    if (!socket) {
        qWarning() << "[MESSAGE PROCESSOR][CLIENT] setSocketMessageHandlers: Socket is null for frequency" << frequency;
        return;
    }

    disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);
    disconnect(socket, &QWebSocket::binaryMessageReceived, this, nullptr);


    const bool connected_text = connect(socket, &QWebSocket::textMessageReceived, this,
                                        [this, frequency](const QString &message) {
                                            ProcessIncomingMessage(message, frequency);
                                        });
    if (!connected_text) {
        qWarning() << "[MESSAGE PROCESSOR][CLIENT] FAILED to connect textMessageReceived for frequency" << frequency;
    }

    const bool connected_binary = connect(socket, &QWebSocket::binaryMessageReceived, this,
                                          [this, frequency](const QByteArray &message) {
                                              ProcessIncomingBinaryMessage(message, frequency);
                                          });

    if (!connected_binary) {
        qWarning() << "[MESSAGE PROCESSOR][CLIENT] FAILED to connect binaryMessageReceived for freqquency" << frequency;
    }

    disconnect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, nullptr);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [socket, frequency](const QAbstractSocket::SocketError error) {
                qWarning() << "[MESSAGE PROCESSOR][CLIENT] WebSocket Error Occurred on freqquency" << frequency
                        << ":" << socket->errorString() << "(Code:" << error << ")";
            });
}

void MessageProcessor::ProcessMessageContent(const QJsonObject &message_object, const QString &frequency,
                                             const QString &message_id) {
    if (MessageHandler::GetInstance()->IsMessageProcessed(message_id)) {
        return;
    }

    MessageHandler::GetInstance()->MarkMessageAsProcessed(message_id);

    const bool has_attachment = message_object.contains("hasAttachment") && message_object["hasAttachment"].toBool();

    if (has_attachment && message_object.contains("attachmentData") && message_object["attachmentData"].toString().
        length() > 100) {
        QJsonObject light_message = message_object;

        const QString attachment_id = AttachmentDataStore::GetInstance()->StoreAttachmentData(
            message_object["attachmentData"].toString());

        light_message["attachmentData"] = attachment_id;

        const QString placeholder_message = MessageFormatter::FormatMessage(light_message, frequency);
        emit messageReceived(frequency, placeholder_message);
    } else {
        const QString formatted_message = MessageFormatter::FormatMessage(message_object, frequency);
        emit messageReceived(frequency, formatted_message);
    }
}

void MessageProcessor::ProcessSystemCommand(const QJsonObject &message_object, const QString &frequency) {
    const QString command = message_object["command"].toString();

    if (command == "close_wavelength") {
        ProcessWavelengthClosed(frequency);
    }
}

void MessageProcessor::ProcessUserJoined(const QJsonObject &message_object, const QString &frequency) {
    const QString user_id = message_object["userId"].toString();
    const QString formatted_message = MessageFormatter::FormatSystemMessage(
        QString("User %1 joined the wavelength").arg(user_id.left(5)));
    emit systemMessage(frequency, formatted_message);
}

void MessageProcessor::ProcessUserLeft(const QJsonObject &message_object, const QString &frequency) {
    const QString user_id = message_object["userId"].toString();
    const QString formatted_message = MessageFormatter::FormatSystemMessage(
        QString("User %1 left the wavelength").arg(user_id.left(5)));
    emit systemMessage(frequency, formatted_message);
}

void MessageProcessor::ProcessWavelengthClosed(const QString &frequency) {
    WavelengthRegistry *registry = WavelengthRegistry::GetInstance();
    if (registry->HasWavelength(frequency)) {
        const QString active_frequency = registry->GetActiveWavelength();
        registry->MarkWavelengthClosing(frequency, true);
        registry->RemoveWavelength(frequency);
        if (active_frequency == frequency) {
            registry->SetActiveWavelength("-1");
        }
        emit wavelengthClosed(frequency);
    }
}

MessageProcessor::MessageProcessor(QObject *parent): QObject(parent) {
    const MessageService *service = MessageService::GetInstance();
    connect(this, &MessageProcessor::pttGranted, service, &MessageService::pttGranted);
    connect(this, &MessageProcessor::pttDenied, service, &MessageService::pttDenied);
    connect(this, &MessageProcessor::pttStartReceiving, service,
            &MessageService::pttStartReceiving);
    connect(this, &MessageProcessor::pttStopReceiving, service, &MessageService::pttStopReceiving);
    connect(this, &MessageProcessor::audioDataReceived, service,
            &MessageService::audioDataReceived);
    connect(this, &MessageProcessor::remoteAudioAmplitudeUpdate, service,
            &MessageService::remoteAudioAmplitudeUpdate);
}
