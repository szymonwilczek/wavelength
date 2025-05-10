#ifndef VIDEO_PLAYER_OVERLAY_H
#define VIDEO_PLAYER_OVERLAY_H

#include <QDialog>
#include <QLabel>
#include <QRandomGenerator>
#include <memory>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

#include "../../../../ui/buttons/cyber_push_button.h"
#include "../../../../ui/sliders/cyber_slider.h"
#include "../decoder/video_decoder.h"

class TranslationManager;
/**
 * @brief A QDialog overlay for playing videos with a cyberpunk aesthetic.
 *
 * This class provides a full-featured video player in a separate dialog window.
 * It uses VideoDecoder for decoding and playback, includes custom cyberpunk-styled
 * UI controls (buttons, sliders), displays video information (codec, resolution, etc.),
 * and incorporates visual effects like scanlines, grid overlays, and random glitches.
 * It handles playback control (play/pause, seek, volume), manages decoder resources,
 * and provides visual feedback on the player's status.
 */
class VideoPlayer final : public QDialog {
    Q_OBJECT
    /** @brief Property controlling the opacity of the scanline effect overlay. Animatable. */
    Q_PROPERTY(double scanlineOpacity READ GetScanlineOpacity WRITE SetScanlineOpacity)
    /** @brief Property controlling the opacity of the background grid effect. Animatable. */
    Q_PROPERTY(double gridOpacity READ GetGridOpacity WRITE SetGridOpacity)

public:
    /**
     * @brief Constructs a VideoPlayerOverlay dialog.
     * Initializes the cyberpunk UI elements, sets window properties (title, size, style),
     * creates timers for UI updates and effects, connects signals for controls,
     * and schedules the player initialization.
     * @param video_data The raw video data to be played.
     * @param mime_type The MIME type of the video data (currently unused).
     * @param parent Optional parent widget.
     */
    explicit VideoPlayer(const QByteArray &video_data, const QString &mime_type, QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops timers and ensures decoder resources are released by calling ReleaseResources().
     */
    ~VideoPlayer() override;

    /**
     * @brief Stops the decoder thread, waits for it to finish, releases its resources, and resets the pointer.
     * Ensures proper cleanup of the VideoDecoder instance.
     */
    void ReleaseResources();

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
        update(); // Trigger repaint when opacity changes
    }

    /**
     * @brief Gets the current opacity value for the background grid effect.
     * @return The grid opacity (0.0 to 1.0).
     */
    double GetGridOpacity() const { return grid_opacity_; }

    /**
     * @brief Sets the opacity for the background grid effect.
     * Triggers an update() to repaint the widget.
     * @param opacity The desired grid opacity (0.0 to 1.0).
     */
    void SetGridOpacity(const double opacity) {
        grid_opacity_ = opacity;
        update(); // Trigger repaint when opacity changes
    }

protected:
    /**
     * @brief Overridden paint event handler. Draws the background grid overlay.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Overridden close event handler.
     * Stops timers and releases decoder resources before closing the dialog.
     * @param event The close event.
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /**
     * @brief Initializes the VideoDecoder instance and connects its signals.
     * Creates the decoder, connects signals for frames, errors, info, finish, and position.
     * Starts the decoder thread. Called automatically after a short delay.
     */
    void InitializePlayer();

    /**
     * @brief Toggles the playback state (play/pause) or resets if finished.
     * If finished, resets the decoder. Otherwise, calls Pause() on the decoder.
     * Updates button text and status label accordingly. Triggers UI animations.
     */
    void TogglePlayback();

    /**
     * @brief Slot called when the progress slider is pressed by the user.
     * Remembers the playback state, pauses playback temporarily, and updates the status label.
     */
    void OnSliderPressed();

    /**
     * @brief Slot called when the progress slider is released by the user.
     * Performs the seek operation based on the slider's final position.
     * Resumes playback if it was active before seeking. Updates status label.
     */
    void OnSliderReleased();

    /**
     * @brief Slot called when the progress slider's value changes (e.g., user dragging).
     * Updates the time label display based on the slider's current position (without seeking yet).
     * @param position The slider's current value (milliseconds).
     */
    void UpdateTimeLabel(int position) const;

    /**
     * @brief Slot connected to the decoder's positionChanged signal.
     * Updates the progress slider's position and the time label display based on the decoder's current playback position.
     * Blocks slider signals during update to prevent feedback loops.
     * @param position The current playback position in seconds.
     */
    void UpdateSliderPosition(double position) const;

    /**
     * @brief Initiates a seek operation in the VideoDecoder.
     * Converts the slider position (milliseconds) to seconds before calling decoder_->Seek().
     * @param position The target position in milliseconds.
     */
    void SeekVideo(int position) const;

    /**
     * @brief Slot connected to the decoder's frameReady signal.
     * Displays the received video frame in the video_label_. Handles scaling, letterboxing,
     * scanline effects, optional HUD elements (corners, timestamp, frame counter), and glitch effects.
     * @param frame The newly decoded video frame.
     */
    void UpdateFrame(const QImage &frame);

    /**
     * @brief Slot called periodically by update_timer_.
     * Updates UI elements like pulsing effects (scanline/grid opacity) and occasionally
     * updates the status label with random "technical" info. Triggers widget repaint.
     */
    void UpdateUI();

    /**
     * @brief Slot connected to the decoder's error signal.
     * Logs the error and displays it in the status_label_ and video_label_.
     * @param message The error message from the decoder.
     */
    void HandleError(const QString &message) const;

    /**
     * @brief Slot connected to the decoder's videoInfo signal.
     * Stores video dimensions, duration, FPS. Sets the progress slider range.
     * Updates info labels (resolution, bitrate, FPS, codec). Adjusts update timer interval.
     * Enables the HUD display. Extracts the first frame for a thumbnail.
     * @param width Video width in pixels.
     * @param height Video height in pixels.
     * @param fps Video frame rate.
     * @param duration Video duration in seconds.
     */
    void HandleVideoInfo(int width, int height, double fps, double duration);

    /**
     * @brief Adjusts the playback volume via the VideoDecoder.
     * Also updates the volume icon based on the new volume level.
     * @param volume The desired volume level (0-100, corresponding to 0.0-1.0 float).
     */
    void AdjustVolume(int volume) const;

    /**
     * @brief Toggles the mute state.
     * If currently unmuted, mutes and remembers the last volume. If muted, restores the last volume.
     */
    void ToggleMute();

    /**
     * @brief Updates the volume button icon based on the current volume level.
     * Shows different icons for muted, low volume, and high volume.
     * @param volume The current volume level (0.0 to 1.0).
     */
    void UpdateVolumeIcon(float volume) const;

    /**
     * @brief Slot called periodically by glitch_timer_.
     * Sets a random intensity for the glitch effect applied in UpdateFrame()
     * and schedules the next glitch event.
     */
    void TriggerGlitch();

private:
    /** @brief QLabel used to display the video frames. */
    QLabel *video_label_;
    /** @brief Custom button for play/pause/replay control. */
    CyberPushButton *play_button_;
    /** @brief Custom slider for displaying and seeking playback progress. */
    CyberSlider *progress_slider_;
    /** @brief Label displaying current time / total duration (MM:SS / MM:SS). */
    QLabel *time_label_;
    /** @brief Custom button for toggling mute. Icon changes based on volume. */
    CyberPushButton *volume_button_;
    /** @brief Custom slider for controlling playback volume. */
    CyberSlider *volume_slider_;
    /** @brief Label displaying playback status (Initializing, Ready, Playing, Paused, Error, Finished). */
    QLabel *status_label_;
    /** @brief Label displaying detected video codec information. */
    QLabel *codec_label_;
    /** @brief Label displaying video resolution. */
    QLabel *resolution_label_;
    /** @brief Label displaying estimated video bitrate. */
    QLabel *bitrate_label_;
    /** @brief Label displaying video frames per second. */
    QLabel *fps_label_;

    /** @brief Shared pointer to the VideoDecoder instance responsible for decoding and playback. */
    std::shared_ptr<VideoDecoder> decoder_;
    /** @brief Timer for triggering periodic UI updates (pulsing effects, status). */
    QTimer *update_timer_;
    /** @brief Timer for triggering random glitch effects. */
    QTimer *glitch_timer_;

    /** @brief Stores the raw video data passed in the constructor. */
    QByteArray video_data_;
    /** @brief Stores the MIME type of the video data (currently unused). */
    QString mime_type_;

    /** @brief Original width of the video in pixels. Set by HandleVideoInfo. */
    int video_width_ = 0;
    /** @brief Original height of the video in pixels. Set by HandleVideoInfo. */
    int video_height_ = 0;
    /** @brief Total duration of the video in seconds. Set by HandleVideoInfo. */
    double video_duration_ = 0;
    /** @brief Flag indicating if the user is currently dragging the progress slider. */
    bool slider_dragging_ = false;
    /** @brief Stores the volume level before muting. */
    int last_volume_ = 100;
    /** @brief Flag indicating if playback has reached the end. */
    bool playback_finished_ = false;
    /** @brief Stores whether playback was active before the user started dragging the slider. */
    bool was_playing_ = false;
    /** @brief Stores the first frame extracted from the video, used as a thumbnail/fallback. */
    QImage thumbnail_frame_;

    /** @brief Current opacity value for the scanline effect (property). */
    double scanline_opacity_;
    /** @brief Current opacity value for the background grid effect (property). */
    double grid_opacity_;
    /** @brief Flag indicating whether to draw the HUD elements (corners, etc.). Enabled after video info is ready. */
    bool show_hud_ = false;
    /** @brief Counter for the frame number displayed in the HUD. */
    int frame_counter_ = 0;
    /** @brief Flag indicating if the decoder has started processing (video info received). */
    bool playback_started_ = false;
    /** @brief Current intensity level for the random glitch effect. */
    double current_glitch_intensity_ = 0.0;
    /** @brief Video frame rate in frames per second. Set by HandleVideoInfo. */
    double video_fps_ = 60.0; // Default FPS
    /** @brief Pointer to the translation manager for handling UI translations. */
    TranslationManager *translator_ = nullptr;
};

#endif //VIDEO_PLAYER_OVERLAY_H
