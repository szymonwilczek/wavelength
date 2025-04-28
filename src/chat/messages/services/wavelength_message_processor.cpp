#include "wavelength_message_processor.h"

#include "../../files/attachments/attachment_data_store.h"
#include "../formatter/message_formatter.h"

void WavelengthMessageProcessor::processIncomingMessage(const QString &message, const QString &frequency) {
    qDebug() << "Processing message for wavelength" << frequency << ":" << message.left(50) + "...";
    bool ok = false;
    const QJsonObject msgObj = MessageHandler::getInstance()->parseMessage(message, &ok);

    if (!ok) {
        qDebug() << "Failed to parse JSON message";
        return;
    }

    const QString msgType = MessageHandler::getInstance()->getMessageType(msgObj);
    const QString messageId = MessageHandler::getInstance()->getMessageId(msgObj);
    const QString msgFrequency = MessageHandler::getInstance()->getMessageFrequency(msgObj);

    qDebug() << "Message type:" << msgType << "ID:" << messageId << "Freq:" << msgFrequency;


    // Verify if the message is for the correct frequency
    if (!areFrequenciesEqual(msgFrequency, frequency) && msgFrequency != -1) {
        qDebug() << "Message frequency mismatch:" << msgFrequency << "vs" << frequency;
        return;
    }

    // Check if message has been processed already
    if (MessageHandler::getInstance()->isMessageProcessed(messageId)) {
        qDebug() << "Message already processed:" << messageId;
        return;
    }

    // Process based on message type
    if (msgType == "message" || msgType == "send_message") {
        processMessageContent(msgObj, frequency, messageId);
    } else if (msgType == "system_command") {
        processSystemCommand(msgObj, frequency);
    } else if (msgType == "user_joined") {
        processUserJoined(msgObj, frequency);
    } else if (msgType == "user_left") {
        processUserLeft(msgObj, frequency);
    } else if (msgType == "wavelength_closed") {
        processWavelengthClosed(msgObj, frequency);
    } else if (msgType == "ptt_granted") {
        emit pttGranted(frequency); // Emituj sygnał do serwisu/koordynatora
    } else if (msgType == "ptt_denied") {
        const QString reason = msgObj.value("reason").toString("Transmission slot is busy.");
        emit pttDenied(frequency, reason); // Emituj sygnał
    } else if (msgType == "ptt_start_receiving") {
        const QString senderId = msgObj.value("senderId").toString("Unknown");
        emit pttStartReceiving(frequency, senderId); // Emituj sygnał
    } else if (msgType == "ptt_stop_receiving") {
        emit pttStopReceiving(frequency); // Emituj sygnał
    } else if (msgType == "audio_amplitude") { // Opcjonalne - jeśli serwer wysyła amplitudę
        const qreal amplitude = msgObj.value("amplitude").toDouble(0.0);
        emit remoteAudioAmplitudeUpdate(frequency, amplitude);
    }
    // --- KONIEC NOWYCH TYPÓW WIADOMOŚCI PTT ---
    else {
        qDebug() << "Unknown message type received:" << msgType;
    }
}

void WavelengthMessageProcessor::processIncomingBinaryMessage(const QByteArray &message, const QString &frequency) {
    qDebug() << "[CLIENT] processIncomingBinaryMessage: Received" << message.size() << "bytes for wavelength" << frequency;
    emit audioDataReceived(frequency, message);
}

void WavelengthMessageProcessor::setSocketMessageHandlers(QWebSocket *socket, QString frequency) {
    if (!socket) {
        qWarning() << "[CLIENT] setSocketMessageHandlers: Socket is null for frequency" << frequency;
        return;
    }

    // Rozłącz stare połączenia, aby uniknąć duplikatów
    disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);
    disconnect(socket, &QWebSocket::binaryMessageReceived, this, nullptr);
    qDebug() << "[CLIENT] Disconnected previous handlers for freq" << frequency;


    // Połącz dla wiadomości tekstowych
    const bool connectedText = connect(socket, &QWebSocket::textMessageReceived, this, [this, frequency](const QString& message) {
        processIncomingMessage(message, frequency);
    });
    if (!connectedText) {
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
    const bool connectedBinary = connect(socket, &QWebSocket::binaryMessageReceived, this, [this, frequency](const QByteArray& message) {
        // Ten log powinien się pojawić, jeśli sygnał zostanie odebrany
        qDebug() << "[CLIENT] binaryMessageReceived signal triggered for freq" << frequency << "Size:" << message.size();
        processIncomingBinaryMessage(message, frequency);
    });

    // --- DODANE LOGOWANIE PO CONNECT DLA BINARY ---
    if (!connectedBinary) {
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

void WavelengthMessageProcessor::processMessageContent(const QJsonObject &msgObj, const QString &frequency,
    const QString &messageId) {
    // Sprawdź czy wiadomość już przetwarzana
    if (MessageHandler::getInstance()->isMessageProcessed(messageId)) {
        qDebug() << "Message already processed, skipping:" << messageId;
        return;
    }

    MessageHandler::getInstance()->markMessageAsProcessed(messageId);

    // Sprawdź, czy wiadomość zawiera załączniki
    const bool hasAttachment = msgObj.contains("hasAttachment") && msgObj["hasAttachment"].toBool();

    if (hasAttachment && msgObj.contains("attachmentData") && msgObj["attachmentData"].toString().length() > 100) {
        // Tworzymy lekką wersję wiadomości z referencją
        QJsonObject lightMsg = msgObj;

        // Zamiast surowych danych base64, zapisujemy dane w magazynie i używamy ID
        const QString attachmentId = AttachmentDataStore::getInstance()->storeAttachmentData(
            msgObj["attachmentData"].toString());

        // Zastępujemy dane załącznika identyfikatorem
        lightMsg["attachmentData"] = attachmentId;

        // Formatujemy i emitujemy placeholder
        const QString placeholderMsg = MessageFormatter::formatMessage(lightMsg, frequency);
        emit messageReceived(frequency, placeholderMsg);
    } else {
        // Dla zwykłych wiadomości tekstowych lub już z referencjami
        const QString formattedMsg = MessageFormatter::formatMessage(msgObj, frequency);
        emit messageReceived(frequency, formattedMsg);
    }
}

void WavelengthMessageProcessor::processSystemCommand(const QJsonObject &msgObj, const QString &frequency) {
    const QString command = msgObj["command"].toString();

    if (command == "ping") {
        qDebug() << "Received ping command";
    } else if (command == "close_wavelength") {
        processWavelengthClosed(msgObj, frequency);
    } else if (command == "kick_user") {
        const QString userId = msgObj["userId"].toString();
        const QString reason = msgObj["reason"].toString("You have been kicked");

        const WavelengthInfo info = WavelengthRegistry::getInstance()->getWavelengthInfo(frequency);
        const QString clientId = info.hostId; // Uproszczenie - potrzeba poprawić

        if (userId == clientId) {
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

void WavelengthMessageProcessor::processUserJoined(const QJsonObject &msgObj, const QString &frequency) {
    const QString userId = msgObj["userId"].toString();
    const QString formattedMsg = MessageFormatter::formatSystemMessage(
        QString("User %1 joined the wavelength").arg(userId.left(5)));
    emit systemMessage(frequency, formattedMsg);
}

void WavelengthMessageProcessor::processUserLeft(const QJsonObject &msgObj, const QString &frequency) {
    const QString userId = msgObj["userId"].toString();
    const QString formattedMsg = MessageFormatter::formatSystemMessage(
        QString("User %1 left the wavelength").arg(userId.left(5)));
    emit systemMessage(frequency, formattedMsg);
}

void WavelengthMessageProcessor::processWavelengthClosed(const QJsonObject &msgObj, const QString &frequency) {
    qDebug() << "Wavelength" << frequency << "was closed";

    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    if (registry->hasWavelength(frequency)) {
        const QString activeFreq = registry->getActiveWavelength();
        registry->markWavelengthClosing(frequency, true);
        registry->removeWavelength(frequency);
        if (activeFreq == frequency) {
            registry->setActiveWavelength("-1");
        }
        emit wavelengthClosed(frequency);
    }
}

WavelengthMessageProcessor::WavelengthMessageProcessor(QObject *parent): QObject(parent) {
    const WavelengthMessageService* service = WavelengthMessageService::getInstance();
    // Połączenia sygnałów z WavelengthMessageService (powinny być OK)
    connect(this, &WavelengthMessageProcessor::pttGranted, service, &WavelengthMessageService::pttGranted);
    connect(this, &WavelengthMessageProcessor::pttDenied, service, &WavelengthMessageService::pttDenied);
    connect(this, &WavelengthMessageProcessor::pttStartReceiving, service, &WavelengthMessageService::pttStartReceiving);
    connect(this, &WavelengthMessageProcessor::pttStopReceiving, service, &WavelengthMessageService::pttStopReceiving);
    connect(this, &WavelengthMessageProcessor::audioDataReceived, service, &WavelengthMessageService::audioDataReceived); // To połączenie jest kluczowe
    connect(this, &WavelengthMessageProcessor::remoteAudioAmplitudeUpdate, service, &WavelengthMessageService::remoteAudioAmplitudeUpdate);

}
