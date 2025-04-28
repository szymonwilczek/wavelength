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

    void processIncomingMessage(const QString& message, QString frequency);

    void processIncomingBinaryMessage(const QByteArray& message, QString frequency);

    void setSocketMessageHandlers(QWebSocket* socket, QString frequency);

private:
    void processMessageContent(const QJsonObject& msgObj, QString frequency, const QString& messageId);

    // Dodaj tę metodę, jeśli jeszcze nie istnieje
    Q_INVOKABLE void emitMessageReceived(QString frequency, const QString& formattedMessage) {
        emit messageReceived(frequency, formattedMessage);
    }

    void processSystemCommand(const QJsonObject& msgObj, QString frequency);

    void processUserJoined(const QJsonObject& msgObj, QString frequency);

    void processUserLeft(const QJsonObject& msgObj, QString frequency);

    void processWavelengthClosed(const QJsonObject& msgObj, QString frequency);

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
    WavelengthMessageProcessor(QObject* parent = nullptr);

    ~WavelengthMessageProcessor() {}

    WavelengthMessageProcessor(const WavelengthMessageProcessor&) = delete;
    WavelengthMessageProcessor& operator=(const WavelengthMessageProcessor&) = delete;
};

#endif // WAVELENGTH_MESSAGE_PROCESSOR_H