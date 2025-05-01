#ifndef WAVELENGTH_STREAM_DISPLAY_H
#define WAVELENGTH_STREAM_DISPLAY_H


#include "../../ui/chat/communication_stream.h"

class WavelengthStreamDisplay final : public QWidget {
    Q_OBJECT

    struct MessageData {
        QString content;
        QString sender;
        QString id;
        StreamMessage::MessageType type;
        bool has_attachment = false;
    };

public:
    explicit WavelengthStreamDisplay(QWidget* parent = nullptr);

    void SetFrequency(const QString &frequency, const QString& name = QString());

    // Główna metoda publiczna do dodawania/aktualizowania wiadomości
    void AddMessage(const QString& message, const QString& message_id, StreamMessage::MessageType type);


    void Clear();

public slots:
    void SetGlitchIntensity(const qreal intensity) const {
        communication_stream_->SetGlitchIntensity(intensity);
    }

    // --- NOWE SLOTY PRZEKAZUJĄCE ---
    void SetTransmittingUser(const QString& userId) const {
        communication_stream_->SetTransmittingUser(userId);
    }

    void ClearTransmittingUser() const {
        communication_stream_->ClearTransmittingUser();
    }

    void SetAudioAmplitude(const qreal amplitude) const {
        communication_stream_->SetAudioAmplitude(amplitude);
    }

private slots:
    void ProcessNextQueuedMessage();

    void OnStreamMessageDestroyed(const QObject* object);

private:
    CommunicationStream* communication_stream_;
    QQueue<MessageData> message_queue_;
    QTimer* message_timer_;
    QMap<QString, StreamMessage*> displayed_progress_messages_;
};

#endif // WAVELENGTH_STREAM_DISPLAY_H