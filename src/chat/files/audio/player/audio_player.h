#ifndef INLINE_AUDIO_PLAYER_H
#define INLINE_AUDIO_PLAYER_H

#include <QLabel>
#include <QPropertyAnimation>

class AudioDecoder;
class CyberAudioSlider;
class CyberAudioButton;
class TranslationManager;

/**
 * @brief An inline audio player widget with a cyberpunk aesthetic.
 *
 * This widget provides playback controls (play/pause, seek, volume) for audio data,
 * displays audio information (type, sample rate, channels), shows the current time
 * and duration, and includes a simple spectrum visualization. It uses AudioDecoder
 * for decoding and playback. It manages an "active player" concept, ensuring only
 * one player is actively using resources (like the audio output device) at a time.
 * Includes visual effects like animated spectrum intensity.
 */
class AudioPlayer final : public QFrame {
    Q_OBJECT
    /** @brief Property controlling the opacity of a potential scanline effect (currently unused in paintEvent). */
    Q_PROPERTY(double scanlineOpacity READ GetScanlineOpacity WRITE SetScanlineOpacity)
    /** @brief Property controlling the intensity/height of the spectrum visualization bars. Animatable. */
    Q_PROPERTY(double spectrumIntensity READ GetSpectrumIntensity WRITE SetSpectrumIntensity)

public:
    /**
     * @brief Constructs an InlineAudioPlayer widget.
     * Initializes UI elements (labels, buttons, sliders, spectrum view), sets up layouts,
     * creates the AudioDecoder instance, connects signals and slots for controls and decoder events,
     * and starts timers for UI updates.
     * @param audio_data The raw audio data to be played.
     * @param mime_type The MIME type of the audio data (e.g., "audio/mpeg").
     * @param parent Optional parent widget.
     */
    explicit AudioPlayer(const QByteArray &audio_data, const QString &mime_type, QWidget *parent = nullptr);

    /**
     * @brief Gets the current opacity value for the scanline effect.
     * @return The scanline opacity (0.0 to 1.0).
     */
    double GetScanlineOpacity() const { return scanline_opacity_; }

    /**
     * @brief Sets the opacity for the scanline effect.
     * Triggers an update() to repaint the widget.
     * @param opacity The desired scanline opacity (0.0 to 1.0).
     */
    void SetScanlineOpacity(const double opacity) {
        scanline_opacity_ = opacity;
        update();
    }

    /**
     * @brief Gets the current intensity multiplier for the spectrum visualization.
     * @return The spectrum intensity value.
     */
    double GetSpectrumIntensity() const { return spectrum_intensity_; }

    /**
     * @brief Sets the intensity multiplier for the spectrum visualization.
     * Triggers an update() to repaint the spectrum view.
     * @param intensity The desired spectrum intensity.
     */
    void SetSpectrumIntensity(const double intensity) {
        spectrum_intensity_ = intensity;
        update();
    }

    /**
     * @brief Destructor. Ensures resources are released.
     */
    ~AudioPlayer() override {
        ReleaseResources();
    }

    /**
     * @brief Stops timers, stops and waits for the decoder thread, and releases decoder resources.
     * Also ensures the global active_player_ pointer is cleared if it points to this instance.
     */
    void ReleaseResources();

    /**
     * @brief Activates this player instance.
     * If another player is active, it deactivates it first. Ensures the decoder is initialized
     * and running. Sets this instance as the globally active player and animates the spectrum intensity up.
     */
    void Activate();

    /**
     * @brief Deactivates this player instance.
     * Pauses playback if active, clears the global active player reference if it points to this instance,
     * and animates the spectrum intensity down.
     */
    void Deactivate();

    /**
     * @brief Adjusts the playback volume via the AudioDecoder.
     * Also updates the volume icon based on the new volume level.
     * @param volume The desired volume level (0-100, corresponding to 0.0-1.0 float).
     */
    void AdjustVolume(int volume) const;

    /**
     * @brief Toggles the mute state.
     * If currently unmuted, mutes and remembers the last volume. If muted, restore the last volume (or 100% if the last volume was 0).
     */
    void ToggleMute();

    /**
     * @brief Updates the volume button icon based on the current volume level.
     * Shows different icons for muted, low volume, and high volume.
     * @param volume The current volume level (0.0 to 1.0).
     */
    void UpdateVolumeIcon(float volume) const;

protected:
    /**
     * @brief Overridden paint event handler. Draws the cyberpunk-style frame border.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Overridden event filter. Intercepts paint events for the spectrum_view_ widget
     *        to call the custom PaintSpectrum method.
     * @param watched The object being watched.
     * @param event The event being processed.
     * @return True if the event was handled (spectrum painted), false otherwise.
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    /**
     * @brief Slot called when the user presses the progress slider.
     * Remembers the playback state and pauses playback temporarily during seeking. Updates status label.
     */
    void OnSliderPressed();

    /**
     * @brief Slot called when the user releases the progress slider.
     * Performs the actual seek operation based on the slider's final position.
     * Resumes playback if it was active before seeking. Updates status label.
     */
    void OnSliderReleased();

    /**
     * @brief Slot called when the progress slider's value changes (e.g., user dragging).
     * Updates the time label display based on the slider's current position (without seeking yet).
     * @param position The slider's current value (milliseconds).
     */
    void UpdateTimeLabel(int position);

    /**
     * @brief Slot connected to the decoder's positionChanged signal.
     * Updates the progress slider's position and the time label display based on the decoder's current playback position.
     * Blocks slider signals during update to prevent feedback loops.
     * @param position The current playback position in seconds.
     */
    void UpdateSliderPosition(double position) const;

    /**
     * @brief Initiates a seek operation in the AudioDecoder.
     * Converts the slider position (milliseconds) to seconds before calling decoder_->Seek().
     * @param position The target position in milliseconds.
     */
    void SeekAudio(int position) const;

    /**
     * @brief Toggles the playback state (play/pause).
     * Activates the player if necessary. Handles restarting playback after finishing.
     * Updates the play button text and status label. Manages spectrum intensity animation.
     */
    void TogglePlayback();

    /**
     * @brief Slot connected to the decoder's error signal.
     * Prints the error message to debug output and updates the status/info labels.
     * @param message The error message from the decoder.
     */
    void HandleError(const QString &message) const;

    /**
     * @brief Slot connected to the decoder's audioInfo signal.
     * Stores the audio duration, sets the range for the progress slider, updates the info label
     * with audio details (type, sample rate, channels), sets status to "READY", and initializes spectrum data.
     * @param sample_rate Audio sample rate in Hz.
     * @param channels Number of audio channels.
     * @param duration Total audio duration in seconds.
     */
    void HandleAudioInfo(int sample_rate, int channels, double duration);

    /**
     * @brief Slot called periodically by ui_timer_.
     * Updates UI elements, primarily the spectrum data for visualization (random fluctuations).
     * It also includes placeholder logic for updating status label text occasionally.
     */
    void UpdateUI();

    /**
     * @brief Starts an animation to increase the spectrum intensity (e.g., when playback starts).
     */
    void IncreaseSpectrumIntensity() {
        const auto spectrum_animation = new QPropertyAnimation(this, "spectrumIntensity");
        spectrum_animation->setDuration(600);
        spectrum_animation->setStartValue(spectrum_intensity_);
        spectrum_animation->setEndValue(0.6);
        spectrum_animation->setEasingCurve(QEasingCurve::OutCubic);
        spectrum_animation->start(QPropertyAnimation::DeleteWhenStopped);
    }

    /**
     * @brief Starts an animation to decrease the spectrum intensity (e.g., when playback pauses or finishes).
     */
    void DecreaseSpectrumIntensity() {
        const auto spectrum_animation = new QPropertyAnimation(this, "spectrumIntensity");
        spectrum_animation->setDuration(800);
        spectrum_animation->setStartValue(spectrum_intensity_);
        spectrum_animation->setEndValue(0.2);
        spectrum_animation->setEasingCurve(QEasingCurve::OutCubic);
        spectrum_animation->start(QPropertyAnimation::DeleteWhenStopped);
    }

private:
    /**
     * @brief Custom painting method for the spectrum visualization widget.
     * Draws a background grid and vertical bars representing audio frequency levels (using spectrum_data_).
     * Called via the event filter on the spectrum_view_ widget.
     * @param target The QWidget (spectrum_view_) to paint onto.
     */
    void PaintSpectrum(QWidget *target);

    /** @brief Label displaying audio type, sample rate, channels. */
    QLabel *audio_info_label_;
    /** @brief Label displaying playback status (Initializing, Ready, Playing, Paused, Error, Finished). */
    QLabel *status_label_;
    /** @brief Widget container where the spectrum visualization is drawn. */
    QWidget *spectrum_view_;
    /** @brief Custom button for play/pause/replay control. */
    CyberAudioButton *play_button_;
    /** @brief Custom slider for displaying and seeking playback progress. */
    CyberAudioSlider *progress_slider_;
    /** @brief Label displaying current time / total duration (MM:SS / MM:SS). */
    QLabel *time_label_;
    /** @brief Custom button for toggling mute. Icon changes based on volume. */
    CyberAudioButton *volume_button_;
    /** @brief Custom slider for controlling playback volume. */
    CyberAudioSlider *volume_slider_;

    /** @brief Shared pointer to the AudioDecoder instance responsible for decoding and playback. */
    std::shared_ptr<AudioDecoder> decoder_;
    /** @brief Timer for triggering periodic UI updates (e.g., spectrum data). */
    QTimer *ui_timer_;
    /** @brief Timer for triggering repaints of the spectrum view. */
    QTimer *spectrum_timer_;

    /** @brief Stores the raw audio data passed in the constructor. */
    QByteArray m_audioData;
    /** @brief Stores the MIME type of the audio data. */
    QString mime_type_;

    /** @brief Total duration of the audio in seconds. Set by HandleAudioInfo. */
    double audio_duration_ = 0;
    /** @brief Current playback position in seconds. Updated by UpdateTimeLabel. */
    double current_position_ = 0;
    /** @brief Flag indicating if the user is currently dragging the progress slider. */
    bool slider_dragging_ = false;
    /** @brief Stores the volume level before muting. */
    int last_volume_ = 100;
    /** @brief Flag indicating if playback has reached the end. */
    bool playback_finished_ = false;
    /** @brief Stores whether playback was active before the user started dragging the slider. */
    bool was_playing_ = false;
    /** @brief Flag indicating if this player instance is the currently active one. */
    bool is_active_ = false;

    /** @brief Opacity value for the scanline effect (property). */
    double scanline_opacity_;
    /** @brief Intensity multiplier for the spectrum visualization (property). */
    double spectrum_intensity_;
    /** @brief Vector storing the current height values for the spectrum bars. Updated by UpdateUI. */
    QVector<double> spectrum_data_;

    /** @brief Static pointer to the currently active InlineAudioPlayer instance. Ensures only one player uses audio output. */
    static inline AudioPlayer *active_player_ = nullptr;

    /** @brief Pointer to the translation manager for handling UI text translations. */
    TranslationManager *translator_;
};

#endif //INLINE_AUDIO_PLAYER_H
