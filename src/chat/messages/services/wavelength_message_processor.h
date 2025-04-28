#ifndef WAVELENGTH_MESSAGE_PROCESSOR_H
#define WAVELENGTH_MESSAGE_PROCESSOR_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <QString>
#include <QDebug>
#include <QDateTime>
#include <qtconcurrentrun.h>

#include "wavelength_message_service.h"
#include "../../files/attachments/attachment_queue_manager.h"
#include "../../../storage/wavelength_registry.h"
#include "../handler/message_handler.h"
#include "../formatter/message_formatter.h"

class WavelengthMessageProcessor : public QObject {
    Q_OBJECT

public:
    static WavelengthMessageProcessor* getInstance() {
        static WavelengthMessageProcessor instance;
        return &instance;
    }

    bool areFrequenciesEqual(QString freq1, QString freq2) {
        return freq1 == freq2;
    }

    void processIncomingMessage(const QString& message, QString frequency) {
        qDebug() << "Processing message for wavelength" << frequency << ":" << message.left(50) + "...";
        bool ok = false;
        QJsonObject msgObj = MessageHandler::getInstance()->parseMessage(message, &ok);

        if (!ok) {
            qDebug() << "Failed to parse JSON message";
            return;
        }

        QString msgType = MessageHandler::getInstance()->getMessageType(msgObj);
        QString messageId = MessageHandler::getInstance()->getMessageId(msgObj);
        QString msgFrequency = MessageHandler::getInstance()->getMessageFrequency(msgObj);

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
            QString reason = msgObj.value("reason").toString("Transmission slot is busy.");
            emit pttDenied(frequency, reason); // Emituj sygnał
        } else if (msgType == "ptt_start_receiving") {
            QString senderId = msgObj.value("senderId").toString("Unknown");
            emit pttStartReceiving(frequency, senderId); // Emituj sygnał
        } else if (msgType == "ptt_stop_receiving") {
            emit pttStopReceiving(frequency); // Emituj sygnał
        } else if (msgType == "audio_amplitude") { // Opcjonalne - jeśli serwer wysyła amplitudę
            qreal amplitude = msgObj.value("amplitude").toDouble(0.0);
            emit remoteAudioAmplitudeUpdate(frequency, amplitude);
        }
        // --- KONIEC NOWYCH TYPÓW WIADOMOŚCI PTT ---
        else {
            qDebug() << "Unknown message type received:" << msgType;
        }
    }

    void processIncomingBinaryMessage(const QByteArray& message, QString frequency) {
        qDebug() << "[CLIENT] processIncomingBinaryMessage: Received" << message.size() << "bytes for wavelength" << frequency;
        emit audioDataReceived(frequency, message);
    }

    void setSocketMessageHandlers(QWebSocket* socket, QString frequency) {
        if (!socket) {
             qWarning() << "[CLIENT] setSocketMessageHandlers: Socket is null for frequency" << frequency;
             return;
        }

        // Rozłącz stare połączenia, aby uniknąć duplikatów
        disconnect(socket, &QWebSocket::textMessageReceived, this, nullptr);
        disconnect(socket, &QWebSocket::binaryMessageReceived, this, nullptr);
        qDebug() << "[CLIENT] Disconnected previous handlers for freq" << frequency;


        // Połącz dla wiadomości tekstowych
        bool connectedText = connect(socket, &QWebSocket::textMessageReceived, this, [this, frequency](const QString& message) {
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
        bool connectedBinary = connect(socket, &QWebSocket::binaryMessageReceived, this, [this, frequency](const QByteArray& message) {
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
                this, [socket, frequency](QAbstractSocket::SocketError error) {
            qWarning() << "[CLIENT] WebSocket Error Occurred on freq" << frequency << ":" << socket->errorString() << "(Code:" << error << ")";
        });


        qDebug() << "Message handlers set for socket on frequency" << frequency; // Istniejący log
    }

private:
    void processMessageContent(const QJsonObject& msgObj, QString frequency, const QString& messageId) {
        // Sprawdź czy wiadomość już przetwarzana
        if (MessageHandler::getInstance()->isMessageProcessed(messageId)) {
            qDebug() << "Message already processed, skipping:" << messageId;
            return;
        }

        MessageHandler::getInstance()->markMessageAsProcessed(messageId);

        // Sprawdź, czy wiadomość zawiera załączniki
        bool hasAttachment = msgObj.contains("hasAttachment") && msgObj["hasAttachment"].toBool();

        if (hasAttachment && msgObj.contains("attachmentData") && msgObj["attachmentData"].toString().length() > 100) {
            // Tworzymy lekką wersję wiadomości z referencją
            QJsonObject lightMsg = msgObj;

            // Zamiast surowych danych base64, zapisujemy dane w magazynie i używamy ID
            QString attachmentId = AttachmentDataStore::getInstance()->storeAttachmentData(
                msgObj["attachmentData"].toString());

            // Zastępujemy dane załącznika identyfikatorem
            lightMsg["attachmentData"] = attachmentId;

            // Formatujemy i emitujemy placeholder
            QString placeholderMsg = MessageFormatter::formatMessage(lightMsg, frequency);
            emit messageReceived(frequency, placeholderMsg);
        } else {
            // Dla zwykłych wiadomości tekstowych lub już z referencjami
            QString formattedMsg = MessageFormatter::formatMessage(msgObj, frequency);
            emit messageReceived(frequency, formattedMsg);
        }
    }

    // Dodaj tę metodę, jeśli jeszcze nie istnieje
    Q_INVOKABLE void emitMessageReceived(QString frequency, const QString& formattedMessage) {
        emit messageReceived(frequency, formattedMessage);
    }

    void processSystemCommand(const QJsonObject& msgObj, QString frequency) {
        QString command = msgObj["command"].toString();

        if (command == "ping") {
            qDebug() << "Received ping command";
        } else if (command == "close_wavelength") {
            processWavelengthClosed(msgObj, frequency);
        } else if (command == "kick_user") {
            QString userId = msgObj["userId"].toString();
            QString reason = msgObj["reason"].toString("You have been kicked");

            WavelengthInfo info = WavelengthRegistry::getInstance()->getWavelengthInfo(frequency);
            QString clientId = info.hostId; // Uproszczenie - potrzeba poprawić

            if (userId == clientId) {
                emit userKicked(frequency, reason);

                // Usuń wavelength z rejestru
                WavelengthRegistry* registry = WavelengthRegistry::getInstance();
                if (registry->hasWavelength(frequency)) {
                    QString activeFreq = registry->getActiveWavelength();
                    registry->removeWavelength(frequency);
                    if (activeFreq == frequency) {
                        registry->setActiveWavelength("-1");
                    }
                }
            }
        }
    }

    void processUserJoined(const QJsonObject& msgObj, QString frequency) {
        QString userId = msgObj["userId"].toString();
        QString formattedMsg = MessageFormatter::formatSystemMessage(
            QString("User %1 joined the wavelength").arg(userId.left(5)));
        emit systemMessage(frequency, formattedMsg);
    }

    void processUserLeft(const QJsonObject& msgObj, QString frequency) {
        QString userId = msgObj["userId"].toString();
        QString formattedMsg = MessageFormatter::formatSystemMessage(
            QString("User %1 left the wavelength").arg(userId.left(5)));
        emit systemMessage(frequency, formattedMsg);
    }

    void processWavelengthClosed(const QJsonObject& msgObj, QString frequency) {
        qDebug() << "Wavelength" << frequency << "was closed";

        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        if (registry->hasWavelength(frequency)) {
            QString activeFreq = registry->getActiveWavelength();
            registry->markWavelengthClosing(frequency, true);
            registry->removeWavelength(frequency);
            if (activeFreq == frequency) {
                registry->setActiveWavelength("-1");
            }
            emit wavelengthClosed(frequency);
        }
    }

signals:
    void messageReceived(QString frequency, const QString& formattedMessage);
    void systemMessage(QString frequency, const QString& formattedMessage);
    void wavelengthClosed(QString frequency);
    void userKicked(QString frequency, const QString& reason);
    void pttGranted(QString frequency);
    void pttDenied(QString frequency, QString reason);
    void pttStartReceiving(QString frequency, QString senderId);
    void pttStopReceiving(QString frequency);
    void audioDataReceived(QString frequency, const QByteArray& audioData);
    void remoteAudioAmplitudeUpdate(QString frequency, qreal amplitude);

private:
    WavelengthMessageProcessor(QObject* parent = nullptr) : QObject(parent) {
        WavelengthMessageService* service = WavelengthMessageService::getInstance();
        // Połączenia sygnałów z WavelengthMessageService (powinny być OK)
        connect(this, &WavelengthMessageProcessor::pttGranted, service, &WavelengthMessageService::pttGranted);
        connect(this, &WavelengthMessageProcessor::pttDenied, service, &WavelengthMessageService::pttDenied);
        connect(this, &WavelengthMessageProcessor::pttStartReceiving, service, &WavelengthMessageService::pttStartReceiving);
        connect(this, &WavelengthMessageProcessor::pttStopReceiving, service, &WavelengthMessageService::pttStopReceiving);
        connect(this, &WavelengthMessageProcessor::audioDataReceived, service, &WavelengthMessageService::audioDataReceived); // To połączenie jest kluczowe
        connect(this, &WavelengthMessageProcessor::remoteAudioAmplitudeUpdate, service, &WavelengthMessageService::remoteAudioAmplitudeUpdate);

    }
    ~WavelengthMessageProcessor() {}

    WavelengthMessageProcessor(const WavelengthMessageProcessor&) = delete;
    WavelengthMessageProcessor& operator=(const WavelengthMessageProcessor&) = delete;
};

#endif // WAVELENGTH_MESSAGE_PROCESSOR_H