#ifndef WAVELENGTH_MESSAGE_SERVICE_H
#define WAVELENGTH_MESSAGE_SERVICE_H

#include <QtConcurrent>

#include "../../../storage/wavelength_registry.h"
#include "../handler/message_handler.h"

class WavelengthMessageService final : public QObject {
    Q_OBJECT

public:
    static WavelengthMessageService* GetInstance() {
        static WavelengthMessageService instance;
        return &instance;
    }

    // --- NOWE METODY PTT ---
    static bool SendPttRequest(const QString& frequency);

    static bool SendPttRelease(const QString& frequency);

    static bool SendAudioData(const QString& frequency, const QByteArray& audio_data);

    bool SendMessage(const QString& message);

    bool SendFile(const QString& file_path, const QString& progress_message_id = QString());

    bool SendFileToServer(const QString& json_message, const QString &frequency, const QString& progress_message_id);


    QMap<QString, QString>* GetSentMessageCache() {
        return &sent_messages_;
    }

    void ClearSentMessageCache() {
        sent_messages_.clear();
    }

    QString GetClientId() const {
        return client_id_;
    }

    void SetClientId(const QString& clientId) {
        client_id_ = clientId;
    }

    public slots:
        void UpdateProgressMessage(const QString& progress_message_id, const QString& message) {
            if (progress_message_id.isEmpty()) return;
            emit progressMessageUpdated(progress_message_id, message);
        }

        void HandleSendJsonViaSocket(const QString& json_message, const QString &frequency, const QString& progress_message_id);

signals:
    void messageSent(QString frequency, const QString& formatted_message);
    void progressMessageUpdated(const QString& message_id, const QString& message);
    void removeProgressMessage(const QString& message_id);
    void sendJsonViaSocket(const QString& json_message, QString frequency, const QString& progress_message_id);
    void pttGranted(QString frequency);
    void pttDenied(QString frequency, QString reason);
    void pttStartReceiving(QString frequency, QString sender_id);
    void pttStopReceiving(QString frequency);
    void audioDataReceived(QString frequency, const QByteArray& audio_data);
    void remoteAudioAmplitudeUpdate(QString frequency, qreal amplitude);

private:
    explicit WavelengthMessageService(QObject* parent = nullptr);

    ~WavelengthMessageService() override {}

    WavelengthMessageService(const WavelengthMessageService&) = delete;
    WavelengthMessageService& operator=(const WavelengthMessageService&) = delete;

    QMap<QString, QString> sent_messages_;
    QString client_id_;

    static QWebSocket* GetSocketForFrequency(const QString& frequency);
};

#endif // WAVELENGTH_MESSAGE_SERVICE_H