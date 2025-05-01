#ifndef WAVELENGTH_CHAT_VIEW_H
#define WAVELENGTH_CHAT_VIEW_H

#include <QAudioInput>
#include <QLineEdit>
#include <QSoundEffect>

#include "../../ui/chat/wavelength_stream_display.h"
#include "../../session/wavelength_session_coordinator.h"
#include "../buttons/cyber_chat_button.h"


class WavelengthChatView final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)

    enum PttState {
        Idle,
        Requesting,
        Transmitting,
        Receiving
    };

public:
    explicit WavelengthChatView(QWidget *parent = nullptr);

    ~WavelengthChatView() override;

    double GetScanlineOpacity() const { return scanline_opacity_; }

    void SetScanlineOpacity(double opacity);


    void SetWavelength(const QString &frequency, const QString &name = QString());

    void OnMessageReceived(const QString &frequency, const QString &message);

    void OnMessageSent(const QString &frequency, const QString &message) const;

    void OnWavelengthClosed(const QString &frequency);

    void Clear();

public slots:
    void AttachFile();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:

    void OnPttButtonPressed();

    void OnPttButtonReleased();

    void OnPttGranted(const QString &frequency);

    void OnPttDenied(const QString &frequency, const QString &reason);

    void OnPttStartReceiving(const QString &frequency, const QString &sender_id);

    void OnPttStopReceiving(const QString &frequency);

    // Slot do obsługi przychodzących danych audio (binarnych)
    void OnAudioDataReceived(const QString &frequency, const QByteArray& audio_data) const;

    // Slot do obsługi danych z mikrofonu
    void OnReadyReadInput() const;

    void UpdateProgressMessage(const QString &message_id, const QString &message) const;

    void SendMessage() const;

    void AbortWavelength();

    void TriggerVisualEffect();

    void TriggerConnectionEffect();

    void TriggerActivityEffect();

signals:
    void wavelengthAborted();

private:
    QLabel *header_label_;
    QLabel *status_indicator_;
    WavelengthStreamDisplay *message_area_;
    QLineEdit *input_field_;
    QPushButton *attach_button_;
    QPushButton *send_button_;
    QPushButton *abort_button_;
    QString current_frequency_ = "-1.0";
    double scanline_opacity_;
    bool is_aborting_ = false;
    QPushButton *ptt_button_;
    QString current_transmitter_id_;

    PttState ptt_state_;
    QAudioInput *audio_input_;
    QAudioOutput *audio_output_;
    QIODevice *input_device_;
    QIODevice *output_device_;
    QAudioFormat audio_format_;
    QSoundEffect *ptt_on_sound_;
    QSoundEffect *ptt_off_sound_;

    void InitializeAudio();

    void StartAudioInput();

    void StopAudioInput();

    void StartAudioOutput();

    void StopAudioOutput();

    qreal CalculateAmplitude(const QByteArray& buffer) const;

    void UpdatePttButtonState() const;

    void ResetStatusIndicator() const;
};

#endif // WAVELENGTH_CHAT_VIEW_H
