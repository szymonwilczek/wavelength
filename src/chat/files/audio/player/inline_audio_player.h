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
    static InlineAudioPlayer* GetActivePlayer() {
        return active_player_;
    }

    InlineAudioPlayer(const QByteArray& audio_data, const QString& mime_type, QWidget* parent = nullptr);

    // Akcesory dla właściwości animacji
    double GetScanlineOpacity() const { return scanline_opacity_; }
    void SetScanlineOpacity(const double opacity) {
        scanline_opacity_ = opacity;
        update();
    }

    double GetSpectrumIntensity() const { return spectrum_intensity_; }
    void SetSpectrumIntensity(const double intensity) {
        spectrum_intensity_ = intensity;
        update();
    }

    ~InlineAudioPlayer() override {
        ReleaseResources();
    }

    void ReleaseResources();

    void Activate();

    void Deactivate();

    void AdjustVolume(int volume) const;

    void ToggleMute();

    void UpdateVolumeIcon(float volume) const;

protected:
    void paintEvent(QPaintEvent* event) override;

    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void OnSliderPressed();

    void OnSliderReleased();

    void UpdateTimeLabel(int position);

    void UpdateSliderPosition(double position) const;

    void SeekAudio(int position) const;

    void TogglePlayback();

    void HandleError(const QString& message) const;

    void HandleAudioInfo(int sample_rate, int channels, double duration);

    void UpdateUI();

    void IncreaseSpectrumIntensity() {
        const auto spectrum_animation = new QPropertyAnimation(this, "spectrumIntensity");
        spectrum_animation->setDuration(600);
        spectrum_animation->setStartValue(spectrum_intensity_);
        spectrum_animation->setEndValue(0.6);
        spectrum_animation->setEasingCurve(QEasingCurve::OutCubic);
        spectrum_animation->start(QPropertyAnimation::DeleteWhenStopped);
    }

    void DecreaseSpectrumIntensity() {
        const auto spectrum_animation = new QPropertyAnimation(this, "spectrumIntensity");
        spectrum_animation->setDuration(800);
        spectrum_animation->setStartValue(spectrum_intensity_);
        spectrum_animation->setEndValue(0.2);
        spectrum_animation->setEasingCurve(QEasingCurve::OutCubic);
        spectrum_animation->start(QPropertyAnimation::DeleteWhenStopped);
    }

private:
    void PaintSpectrum(QWidget* target);

    // Pola klasy InlineAudioPlayer
    QLabel* audio_info_label_;
    QLabel* status_label_;
    QWidget* spectrum_view_;
    CyberAudioButton* play_button_;
    CyberAudioSlider* progress_slider_;
    QLabel* time_label_;
    CyberAudioButton* volume_button_;
    CyberAudioSlider* volume_slider_;

    std::shared_ptr<AudioDecoder> decoder_;
    QTimer* ui_timer_;
    QTimer* spectrum_timer_;

    QByteArray m_audioData;
    QString mime_type_;

    double audio_duration_ = 0;
    double current_position_ = 0;
    bool slider_dragging_ = false;
    int last_volume_ = 100;
    bool playback_finished_ = false;
    bool was_playing_ = false;
    bool is_active_ = false;

    double scanline_opacity_;
    double spectrum_intensity_;
    QVector<double> spectrum_data_;
    
    static inline InlineAudioPlayer* active_player_ = nullptr;
};

#endif //INLINE_AUDIO_PLAYER_H