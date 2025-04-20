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

#include "../files/attachment_queue_manager.h"
#include "../registry/wavelength_registry.h"
#include "../messages/handler/message_handler.h"
#include "../messages/formatter/message_formatter.h"

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
        } else {
            qDebug() << "Unknown message type:" << msgType;
        }
    }

    void setSocketMessageHandlers(QWebSocket* socket, QString frequency) {
        if (!socket) return;

        connect(socket, &QWebSocket::textMessageReceived, this, [this, frequency](const QString& message) {
            qDebug() << "Message processor received message for freq" << frequency << ":" << message.left(50) + "...";
            processIncomingMessage(message, frequency);
        });
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

private:
    WavelengthMessageProcessor(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthMessageProcessor() {}

    WavelengthMessageProcessor(const WavelengthMessageProcessor&) = delete;
    WavelengthMessageProcessor& operator=(const WavelengthMessageProcessor&) = delete;
};

#endif // WAVELENGTH_MESSAGE_PROCESSOR_H