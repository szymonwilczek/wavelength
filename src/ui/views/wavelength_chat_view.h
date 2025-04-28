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

    double scanlineOpacity() const { return m_scanlineOpacity; }

    void setScanlineOpacity(double opacity);


    void setWavelength(const QString &frequency, const QString &name = QString());

    void onMessageReceived(const QString &frequency, const QString &message);

    void onMessageSent(const QString &frequency, const QString &message) const;

    void onWavelengthClosed(const QString &frequency);

    void clear();

public slots:
    void attachFile();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:

    void onPttButtonPressed();

    void onPttButtonReleased();

    void onPttGranted(const QString &frequency);

    void onPttDenied(const QString &frequency, const QString &reason);

    void onPttStartReceiving(const QString &frequency, const QString &senderId);

    void onPttStopReceiving(const QString &frequency);

    // Slot do obsługi przychodzących danych audio (binarnych)
    void onAudioDataReceived(const QString &frequency, const QByteArray& audioData) const;

    // Slot do obsługi danych z mikrofonu
    void onReadyReadInput() const;


    void updateProgressMessage(const QString &messageId, const QString &message) const;


    void sendMessage() const;

    void abortWavelength();

    void triggerVisualEffect();

    void triggerConnectionEffect();

    void triggerActivityEffect();

signals:
    void wavelengthAborted();

private:
    QLabel *headerLabel;
    QLabel *m_statusIndicator;
    WavelengthStreamDisplay *messageArea;
    QLineEdit *inputField;
    QPushButton *attachButton;
    QPushButton *sendButton;
    QPushButton *abortButton;
    QString currentFrequency = "-1.0";
    double m_scanlineOpacity;
    bool m_isAborting = false;
    QPushButton *pttButton; // <-- Nowy przycisk
    QString m_currentTransmitterId;

    // --- NOWE POLA AUDIO ---
    PttState m_pttState;
    QAudioInput *m_audioInput;
    QAudioOutput *m_audioOutput;
    QIODevice *m_inputDevice;
    QIODevice *m_outputDevice;
    QAudioFormat m_audioFormat;
    QSoundEffect *m_pttOnSound;
    QSoundEffect *m_pttOffSound;
    // --- KONIEC NOWYCH PÓL AUDIO ---

    // --- NOWE METODY PRYWATNE ---
    void initializeAudio();

    void startAudioInput();

    void stopAudioInput();

    void startAudioOutput();

    void stopAudioOutput();

    // Obliczanie amplitudy RMS z danych PCM 16-bit
    qreal calculateAmplitude(const QByteArray& buffer) const;

    // Aktualizacja stanu przycisku PTT i innych kontrolek
    void updatePttButtonState() const;

    // --- KONIEC NOWYCH METOD PRYWATNYCH ---

    void resetStatusIndicator() const;
};

#endif // WAVELENGTH_CHAT_VIEW_H
