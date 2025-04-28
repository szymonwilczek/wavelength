#ifndef INLINE_AUDIO_PLAYER_H
#define INLINE_AUDIO_PLAYER_H

#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <memory>
#include <QApplication>

#include "../../../../ui/buttons/cyber_audio_button.h"
#include "../../../../ui/sliders/cyber_audio_slider.h"
#include "../../audio/decoder/audio_decoder.h"


class InlineAudioPlayer final : public QFrame {
    Q_OBJECT
    Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
    Q_PROPERTY(double spectrumIntensity READ spectrumIntensity WRITE setSpectrumIntensity)

public:
    static InlineAudioPlayer* getActivePlayer() {
        return s_activePlayer;
    }

    InlineAudioPlayer(const QByteArray& audioData, const QString& mimeType, QWidget* parent = nullptr);

    // Akcesory dla właściwości animacji
    double scanlineOpacity() const { return m_scanlineOpacity; }
    void setScanlineOpacity(const double opacity) {
        m_scanlineOpacity = opacity;
        update();
    }

    double spectrumIntensity() const { return m_spectrumIntensity; }
    void setSpectrumIntensity(const double intensity) {
        m_spectrumIntensity = intensity;
        update();
    }

    ~InlineAudioPlayer() override {
        releaseResources();
    }

    void releaseResources();

    void activate();

    void deactivate();

    void adjustVolume(int volume) const;

    void toggleMute();

    void updateVolumeIcon(float volume) const;

protected:
    void paintEvent(QPaintEvent* event) override;

    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onSliderPressed();

    void onSliderReleased();

    void updateTimeLabel(int position);

    void updateSliderPosition(double position) const;

    void seekAudio(int position) const;

    void togglePlayback();

    void handleError(const QString& message) const;

    void handleAudioInfo(int sampleRate, int channels, double duration);

    void updateUI();

    void increaseSpectrumIntensity() {
        const auto spectrumAnim = new QPropertyAnimation(this, "spectrumIntensity");
        spectrumAnim->setDuration(600);
        spectrumAnim->setStartValue(m_spectrumIntensity);
        spectrumAnim->setEndValue(0.6);
        spectrumAnim->setEasingCurve(QEasingCurve::OutCubic);
        spectrumAnim->start(QPropertyAnimation::DeleteWhenStopped);
    }

    void decreaseSpectrumIntensity() {
        const auto spectrumAnim = new QPropertyAnimation(this, "spectrumIntensity");
        spectrumAnim->setDuration(800);
        spectrumAnim->setStartValue(m_spectrumIntensity);
        spectrumAnim->setEndValue(0.2);
        spectrumAnim->setEasingCurve(QEasingCurve::OutCubic);
        spectrumAnim->start(QPropertyAnimation::DeleteWhenStopped);
    }

private:
    void paintSpectrum(QWidget* target);

    // Pola klasy InlineAudioPlayer
    QLabel* m_audioInfoLabel;
    QLabel* m_statusLabel;
    QWidget* m_spectrumView;
    CyberAudioButton* m_playButton;
    CyberAudioSlider* m_progressSlider;
    QLabel* m_timeLabel;
    CyberAudioButton* m_volumeButton;
    CyberAudioSlider* m_volumeSlider;

    std::shared_ptr<AudioDecoder> m_decoder;
    QTimer* m_uiTimer;
    QTimer* m_spectrumTimer;

    QByteArray m_audioData;
    QString m_mimeType;

    double m_audioDuration = 0;
    double m_currentPosition = 0;
    bool m_sliderDragging = false;
    int m_lastVolume = 100;
    bool m_playbackFinished = false;
    bool m_wasPlaying = false;
    bool m_isActive = false;

    // Właściwości animacji
    double m_scanlineOpacity;
    double m_spectrumIntensity;
    QVector<double> m_spectrumData;
    
    static inline InlineAudioPlayer* s_activePlayer = nullptr;
};

#endif //INLINE_AUDIO_PLAYER_H