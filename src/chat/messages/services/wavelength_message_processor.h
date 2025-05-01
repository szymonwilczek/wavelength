#ifndef WAVELENGTH_MESSAGE_PROCESSOR_H
#define WAVELENGTH_MESSAGE_PROCESSOR_H

#include "wavelength_message_service.h"
#include "../handler/message_handler.h"

class WavelengthMessageProcessor final : public QObject {
    Q_OBJECT

public:
    static WavelengthMessageProcessor* GetInstance() {
        static WavelengthMessageProcessor instance;
        return &instance;
    }

    static bool AreFrequenciesEqual(const QString &frequency1, const QString &frequency2) {
        return frequency1 == frequency2;
    }

    void ProcessIncomingMessage(const QString& message, const QString &frequency);

    void ProcessIncomingBinaryMessage(const QByteArray& message, const QString &frequency);

    void SetSocketMessageHandlers(QWebSocket* socket, QString frequency);

private:
    void ProcessMessageContent(const QJsonObject& message_object, const QString &frequency, const QString& message_id);

    Q_INVOKABLE void EmitMessageReceived(const QString &frequency, const QString& formatted_message) {
        emit messageReceived(frequency, formatted_message);
    }

    void ProcessSystemCommand(const QJsonObject& message_object, const QString &frequency);

    void ProcessUserJoined(const QJsonObject& message_object, const QString &frequency);

    void ProcessUserLeft(const QJsonObject& message_object, const QString &frequency);

    void ProcessWavelengthClosed(const QString &frequency);

signals:
    void messageReceived(QString frequency, const QString& formatted_message);
    void systemMessage(QString frequency, const QString& formatted_message);
    void wavelengthClosed(QString frequency);
    void userKicked(QString frequency, const QString& reason);
    void pttGranted(QString frequency);
    void pttDenied(QString frequency, QString reason);
    void pttStartReceiving(QString frequency, QString sender_id);
    void pttStopReceiving(QString frequency);
    void audioDataReceived(QString frequency, const QByteArray& audio_data);
    void remoteAudioAmplitudeUpdate(QString frequency, qreal amplitude);

private:
    explicit WavelengthMessageProcessor(QObject* parent = nullptr);

    ~WavelengthMessageProcessor() override {}

    WavelengthMessageProcessor(const WavelengthMessageProcessor&) = delete;
    WavelengthMessageProcessor& operator=(const WavelengthMessageProcessor&) = delete;
};

#endif // WAVELENGTH_MESSAGE_PROCESSOR_H