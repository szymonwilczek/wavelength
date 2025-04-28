#ifndef WAVELENGTH_MESSAGE_PROCESSOR_H
#define WAVELENGTH_MESSAGE_PROCESSOR_H

#include "wavelength_message_service.h"
#include "../handler/message_handler.h"

class WavelengthMessageProcessor final : public QObject {
    Q_OBJECT

public:
    static WavelengthMessageProcessor* getInstance() {
        static WavelengthMessageProcessor instance;
        return &instance;
    }

    static bool areFrequenciesEqual(const QString &freq1, const QString &freq2) {
        return freq1 == freq2;
    }

    void processIncomingMessage(const QString& message, const QString &frequency);

    void processIncomingBinaryMessage(const QByteArray& message, const QString &frequency);

    void setSocketMessageHandlers(QWebSocket* socket, QString frequency);

private:
    void processMessageContent(const QJsonObject& msgObj, const QString &frequency, const QString& messageId);

    // Dodaj tę metodę, jeśli jeszcze nie istnieje
    Q_INVOKABLE void emitMessageReceived(const QString &frequency, const QString& formattedMessage) {
        emit messageReceived(frequency, formattedMessage);
    }

    void processSystemCommand(const QJsonObject& msgObj, const QString &frequency);

    void processUserJoined(const QJsonObject& msgObj, const QString &frequency);

    void processUserLeft(const QJsonObject& msgObj, const QString &frequency);

    void processWavelengthClosed(const QJsonObject& msgObj, const QString &frequency);

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
    explicit WavelengthMessageProcessor(QObject* parent = nullptr);

    ~WavelengthMessageProcessor() override {}

    WavelengthMessageProcessor(const WavelengthMessageProcessor&) = delete;
    WavelengthMessageProcessor& operator=(const WavelengthMessageProcessor&) = delete;
};

#endif // WAVELENGTH_MESSAGE_PROCESSOR_H