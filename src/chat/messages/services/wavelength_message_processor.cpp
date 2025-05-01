#include "wavelength_message_processor.h"

#include "../../files/attachments/attachment_data_store.h"
#include "../formatter/message_formatter.h"

void WavelengthMessageProcessor::ProcessIncomingMessage(const QString &message, const QString &frequency) {
    qDebug() << "Processing message for wavelength" << frequency << ":" << message.left(50) + "...";
    bool ok = false;
    const QJsonObject message_object = MessageHandler::GetInstance()->ParseMessage(message, &ok);

    if (!ok) {
        qDebug() << "Failed to parse JSON message";
        return;
    }

    const QString message_type = MessageHandler::GetInstance()->GetMessageType(message_object);
    const QString message_id = MessageHandler::GetInstance()->GetMessageId(message_object);
    const QString message_frequency = MessageHandler::GetInstance()->GetMessageFrequency(message_object);

    qDebug() << "Message type:" << message_type << "ID:" << message_id << "Freq:" << message_frequency;


    // Verify if the message is for the correct frequency
    if (!AreFrequenciesEqual(message_frequency, frequency) && message_frequency != -1) {
        qDebug() << "Message frequency mismatch:" << message_frequency << "vs" << frequency;
        return;
    }

    // Check if message has been processed already
    if (MessageHandler::GetInstance()->IsMessageProcessed(message_id)) {
        qDebug() << "Message already processed:" << message_id;
        return;
    }

    // Process based on message type
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
        emit pttGranted(frequency); // Emituj sygnał do serwisu/koordynatora
    } else if (message_type == "ptt_denied") {
        const QString reason = message_object.value("reason").toString("Transmission slot is busy.");
        emit pttDenied(frequency, reason); // Emituj sygnał
    } else if (message_type == "ptt_start_receiving") {
        const QString senderId = message_object.value("senderId").toString("Unknown");
        emit pttStartReceiving(frequency, senderId); // Emituj sygnał
    } else if (message_type == "ptt_stop_receiving") {
        emit pttStopReceiving(frequency); // Emituj sygnał
    } else if (message_type == "audio_amplitude") { // Opcjonalne - jeśli serwer wysyła amplitudę
        const qreal amplitude = message_object.value("amplitude").toDouble(0.0);
        emit remoteAudioAmplitudeUpdate(frequency, amplitude);
    }
    // --- KONIEC NOWYCH TYPÓW WIADOMOŚCI PTT ---
    else {
        qDebug() << "Unknown message type received:" << message_type;
    }
}

void WavelengthMessageProcessor::ProcessIncomingBinaryMessage(const QByteArray &message, const QString &frequency) {
    qDebug() << "[CLIENT] processIncomingBinaryMessage: Received" << message.size() << "bytes for wavelength" << frequency;
    emit audioDataReceived(frequency, message);
}

void WavelengthMessageProcessor::SetSocketMessageHandlers(QWebSocket *socket, QString frequency) {
    if (!socket) {
        qWarning() << "[CLIENT] setSocketMessageHandlers: Socket is null for frequency" << frequency;
        return;
    }

    // Rozłącz stare połączenia, aby uniknąć duplikatów
    disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);
    disconnect(socket, &QWebSocket::binaryMessageReceived, this, nullptr);
    qDebug() << "[CLIENT] Disconnected previous handlers for freq" << frequency;


    // Połącz dla wiadomości tekstowych
    const bool connected_text = connect(socket, &QWebSocket::textMessageReceived, this, [this, frequency](const QString& message) {
        ProcessIncomingMessage(message, frequency);
    });
    if (!connected_text) {
        qWarning() << "[CLIENT] FAILED to connect textMessageReceived for freq" << frequency;
    } else {
        qDebug() << "[CLIENT] Successfully connected textMessageReceived for freq" << frequency;
    }


    // --- DODANE LOGOWANIE PRZED CONNECT DLA BINARY ---
    qDebug() << "[CLIENT] Attempting to connect binaryMessageReceived for freq" << frequency
            << "Socket valid:" << socket->isValid()
            << "State:" << socket->state();
    // --- KONIEC LOGOWANIA ---

    // Połącz dla wiadomości binarnych
    const bool connected_binary = connect(socket, &QWebSocket::binaryMessageReceived, this, [this, frequency](const QByteArray& message) {
        // Ten log powinien się pojawić, jeśli sygnał zostanie odebrany
        qDebug() << "[CLIENT] binaryMessageReceived signal triggered for freq" << frequency << "Size:" << message.size();
        ProcessIncomingBinaryMessage(message, frequency);
    });

    // --- DODANE LOGOWANIE PO CONNECT DLA BINARY ---
    if (!connected_binary) {
        qWarning() << "[CLIENT] FAILED to connect binaryMessageReceived for freq" << frequency;
    } else {
        qDebug() << "[CLIENT] Successfully connected binaryMessageReceived for freq" << frequency;
    }
    // --- KONIEC LOGOWANIA ---

    // Dodajmy też logowanie błędów samego socketa na wszelki wypadek
    // Rozłączamy najpierw, aby uniknąć wielokrotnego podłączania tego samego slotu
    disconnect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, nullptr);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [socket, frequency](const QAbstractSocket::SocketError error) {
                qWarning() << "[CLIENT] WebSocket Error Occurred on freq" << frequency << ":" << socket->errorString() << "(Code:" << error << ")";
            });


    qDebug() << "Message handlers set for socket on frequency" << frequency; // Istniejący log
}

void WavelengthMessageProcessor::ProcessMessageContent(const QJsonObject &message_object, const QString &frequency,
    const QString &message_id) {
    // Sprawdź czy wiadomość już przetwarzana
    if (MessageHandler::GetInstance()->IsMessageProcessed(message_id)) {
        qDebug() << "Message already processed, skipping:" << message_id;
        return;
    }

    MessageHandler::GetInstance()->MarkMessageAsProcessed(message_id);

    // Sprawdź, czy wiadomość zawiera załączniki
    const bool has_attachment = message_object.contains("hasAttachment") && message_object["hasAttachment"].toBool();

    if (has_attachment && message_object.contains("attachmentData") && message_object["attachmentData"].toString().length() > 100) {
        // Tworzymy lekką wersję wiadomości z referencją
        QJsonObject light_message = message_object;

        // Zamiast surowych danych base64, zapisujemy dane w magazynie i używamy ID
        const QString attachment_id = AttachmentDataStore::GetInstance()->StoreAttachmentData(
            message_object["attachmentData"].toString());

        // Zastępujemy dane załącznika identyfikatorem
        light_message["attachmentData"] = attachment_id;

        // Formatujemy i emitujemy placeholder
        const QString placeholder_message = MessageFormatter::FormatMessage(light_message, frequency);
        emit messageReceived(frequency, placeholder_message);
    } else {
        // Dla zwykłych wiadomości tekstowych lub już z referencjami
        const QString formatted_message = MessageFormatter::FormatMessage(message_object, frequency);
        emit messageReceived(frequency, formatted_message);
    }
}

void WavelengthMessageProcessor::ProcessSystemCommand(const QJsonObject &message_object, const QString &frequency) {
    const QString command = message_object["command"].toString();

    if (command == "ping") {
        qDebug() << "Received ping command";
    } else if (command == "close_wavelength") {
        ProcessWavelengthClosed(frequency);
    } else if (command == "kick_user") {
        const QString user_id = message_object["userId"].toString();
        const QString reason = message_object["reason"].toString("You have been kicked");

        const WavelengthInfo info = WavelengthRegistry::getInstance()->getWavelengthInfo(frequency);
        const QString client_id = info.hostId; // Uproszczenie - potrzeba poprawić

        if (user_id == client_id) {
            emit userKicked(frequency, reason);

            // Usuń wavelength z rejestru
            WavelengthRegistry* registry = WavelengthRegistry::getInstance();
            if (registry->hasWavelength(frequency)) {
                const QString activeFreq = registry->getActiveWavelength();
                registry->removeWavelength(frequency);
                if (activeFreq == frequency) {
                    registry->setActiveWavelength("-1");
                }
            }
        }
    }
}

void WavelengthMessageProcessor::ProcessUserJoined(const QJsonObject &message_object, const QString &frequency) {
    const QString user_id = message_object["userId"].toString();
    const QString formatted_message = MessageFormatter::FormatSystemMessage(
        QString("User %1 joined the wavelength").arg(user_id.left(5)));
    emit systemMessage(frequency, formatted_message);
}

void WavelengthMessageProcessor::ProcessUserLeft(const QJsonObject &message_object, const QString &frequency) {
    const QString user_id = message_object["userId"].toString();
    const QString formatted_message = MessageFormatter::FormatSystemMessage(
        QString("User %1 left the wavelength").arg(user_id.left(5)));
    emit systemMessage(frequency, formatted_message);
}

void WavelengthMessageProcessor::ProcessWavelengthClosed(const QString &frequency) {
    qDebug() << "Wavelength" << frequency << "was closed";

    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    if (registry->hasWavelength(frequency)) {
        const QString active_frequency = registry->getActiveWavelength();
        registry->markWavelengthClosing(frequency, true);
        registry->removeWavelength(frequency);
        if (active_frequency == frequency) {
            registry->setActiveWavelength("-1");
        }
        emit wavelengthClosed(frequency);
    }
}

WavelengthMessageProcessor::WavelengthMessageProcessor(QObject *parent): QObject(parent) {
    const WavelengthMessageService* service = WavelengthMessageService::GetInstance();
    connect(this, &WavelengthMessageProcessor::pttGranted, service, &WavelengthMessageService::pttGranted);
    connect(this, &WavelengthMessageProcessor::pttDenied, service, &WavelengthMessageService::pttDenied);
    connect(this, &WavelengthMessageProcessor::pttStartReceiving, service, &WavelengthMessageService::pttStartReceiving);
    connect(this, &WavelengthMessageProcessor::pttStopReceiving, service, &WavelengthMessageService::pttStopReceiving);
    connect(this, &WavelengthMessageProcessor::audioDataReceived, service, &WavelengthMessageService::audioDataReceived);
    connect(this, &WavelengthMessageProcessor::remoteAudioAmplitudeUpdate, service, &WavelengthMessageService::remoteAudioAmplitudeUpdate);
}
