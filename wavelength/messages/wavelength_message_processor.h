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

    bool areFrequenciesEqual(double freq1, double freq2, double epsilon = 0.0001) {
        return std::abs(freq1 - freq2) < epsilon;
    }

    void processIncomingMessage(const QString& message, double frequency) {
        qDebug() << "Processing message for wavelength" << frequency << ":" << message.left(50) + "...";
        bool ok = false;
        QJsonObject msgObj = MessageHandler::getInstance()->parseMessage(message, &ok);

        if (!ok) {
            qDebug() << "Failed to parse JSON message";
            return;
        }

        QString msgType = MessageHandler::getInstance()->getMessageType(msgObj);
        QString messageId = MessageHandler::getInstance()->getMessageId(msgObj);
        double msgFrequency = MessageHandler::getInstance()->getMessageFrequency(msgObj);

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

    void setSocketMessageHandlers(QWebSocket* socket, double frequency) {
        if (!socket) return;

        connect(socket, &QWebSocket::textMessageReceived, this, [this, frequency](const QString& message) {
            qDebug() << "Message processor received message for freq" << frequency << ":" << message.left(50) + "...";
            processIncomingMessage(message, frequency);
        });
    }

private:
    void processMessageContent(const QJsonObject& msgObj, double frequency, const QString& messageId) {
        // Sprawdź czy wiadomość już przetwarzana
        if (MessageHandler::getInstance()->isMessageProcessed(messageId)) {
            qDebug() << "Message already processed, skipping:" << messageId;
            return;
        }

        MessageHandler::getInstance()->markMessageAsProcessed(messageId);

        // Sprawdź, czy wiadomość zawiera duże załączniki
        bool hasAttachment = msgObj.contains("hasAttachment") && msgObj["hasAttachment"].toBool();

        if (hasAttachment && msgObj.contains("attachmentData")) {
            // Dla dużych załączników, formatuj i emituj sygnał asynchronicznie
            QtConcurrent::run([this, msgObj, frequency, messageId]() {
                QString formattedMsg = MessageFormatter::formatMessage(msgObj, frequency);
                QMetaObject::invokeMethod(this, "emitMessageReceived",
                    Qt::QueuedConnection,
                    Q_ARG(double, frequency),
                    Q_ARG(QString, formattedMsg));
            });
        } else {
            // Dla zwykłych wiadomości tekstowych, formatuj standardowo
            QString formattedMsg = MessageFormatter::formatMessage(msgObj, frequency);
            emit messageReceived(frequency, formattedMsg);
        }
    }

    // Dodaj metodę dla emitowania sygnału w głównym wątku:
    Q_INVOKABLE void emitMessageReceived(double frequency, const QString& formattedMessage) {
        emit messageReceived(frequency, formattedMessage);
    }

    void processSystemCommand(const QJsonObject& msgObj, double frequency) {
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
                    int activeFreq = registry->getActiveWavelength();
                    registry->removeWavelength(frequency);
                    if (activeFreq == frequency) {
                        registry->setActiveWavelength(-1);
                    }
                }
            }
        }
    }

    void processUserJoined(const QJsonObject& msgObj, double frequency) {
        QString userId = msgObj["userId"].toString();
        QString formattedMsg = MessageFormatter::formatSystemMessage(
            QString("User %1 joined the wavelength").arg(userId.left(5)));
        emit systemMessage(frequency, formattedMsg);
    }

    void processUserLeft(const QJsonObject& msgObj, double frequency) {
        QString userId = msgObj["userId"].toString();
        QString formattedMsg = MessageFormatter::formatSystemMessage(
            QString("User %1 left the wavelength").arg(userId.left(5)));
        emit systemMessage(frequency, formattedMsg);
    }

    void processWavelengthClosed(const QJsonObject& msgObj, double frequency) {
        qDebug() << "Wavelength" << frequency << "was closed";

        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        if (registry->hasWavelength(frequency)) {
            int activeFreq = registry->getActiveWavelength();
            registry->markWavelengthClosing(frequency, true);
            registry->removeWavelength(frequency);
            if (activeFreq == frequency) {
                registry->setActiveWavelength(-1);
            }
            emit wavelengthClosed(frequency);
        }
    }

signals:
    void messageReceived(double frequency, const QString& formattedMessage);
    void systemMessage(double frequency, const QString& formattedMessage);
    void wavelengthClosed(double frequency);
    void userKicked(double frequency, const QString& reason);

private:
    WavelengthMessageProcessor(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthMessageProcessor() {}

    WavelengthMessageProcessor(const WavelengthMessageProcessor&) = delete;
    WavelengthMessageProcessor& operator=(const WavelengthMessageProcessor&) = delete;
};

#endif // WAVELENGTH_MESSAGE_PROCESSOR_H