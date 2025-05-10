#ifndef INLINE_GIF_PLAYER_H
#define INLINE_GIF_PLAYER_H

#include <memory>
#include <QLabel>

#include "../decoder/gif_decoder.h"

class TranslationManager;
/**
 * @brief An inline widget for displaying and playing animated GIFs within a chat interface.
 *
 * This class uses GifDecoder to decode GIF data in a separate thread. It displays
 * the first frame as a static thumbnail initially. When the user hovers the mouse
 * over the widget, playback starts automatically. Playback pauses when the mouse leaves.
 * The widget scales the displayed GIF while maintaining an aspect ratio.
 */
class InlineGifPlayer final : public QFrame {
    Q_OBJECT

public:
    /**
     * @brief Constructs an InlineGifPlayer widget.
     * Initializes the UI (QLabel for display), creates the GifDecoder instance,
     * connects signals for frame updates, errors, and GIF info, and triggers
     * asynchronous initialization of the decoder to load the first frame.
     * @param gif_data The raw GIF data to be played.
     * @param parent Optional parent widget.
     */
    explicit InlineGifPlayer(const QByteArray &gif_data, QWidget *parent = nullptr);

    /**
     * @brief Destructor. Ensures resources are released by calling ReleaseResources().
     */
    ~InlineGifPlayer() override {
        ReleaseResources();
    }

    /**
     * @brief Stops the decoder thread, waits for it to finish (with timeout), and resets the decoder pointer.
     * Ensures proper cleanup, especially when the widget is destroyed or the application quits.
     */
    void ReleaseResources();

    /**
     * @brief Provides a size hint based on the original dimensions of the GIF.
     * Returns the actual GIF width and height once known, otherwise falls back to the default QFrame size hint.
     * @return The recommended QSize for the widget.
     */
    QSize sizeHint() const override;

    /**
     * @brief Starts or resumes GIF playback.
     * Sets the is_playing_ flag and calls Resume() on the GifDecoder.
     * Typically called automatically on mouse enter.
     */
    void StartPlayback();

    /**
     * @brief Stops or pauses GIF playback.
     * Clears the is_playing_ flag and calls Pause() on the GifDecoder.
     * Typically called automatically on mouse leave.
     */
    void StopPlayback();

protected:
    /**
     * @brief Handles mouse enter events.
     * Calls StartPlayback() to begin animation when the mouse cursor enters the widget area.
     * @param event The enter event.
     */
    void enterEvent(QEvent *event) override;

    /**
     * @brief Handles mouse leave events.
     * Calls StopPlayback() to pause animation when the mouse cursor leaves the widget area.
     * @param event The leave event.
     */
    void leaveEvent(QEvent *event) override;

private slots:
    /**
     * @brief Slot connected to the decoder's firstFrameReady signal.
     * Displays the provided frame as a static thumbnail in the QLabel.
     * Stores the pixmap for later use (e.g., when pausing).
     * @param frame The first decoded frame of the GIF.
     */
    void DisplayThumbnail(const QImage &frame);

    /**
     * @brief Slot connected to the decoder's frameReady signal.
     * Updates the QLabel to display the new frame, but only if playback is currently active (is_playing_ is true).
     * Relies on QLabel's setScaledContents(true) for rendering.
     * @param frame The newly decoded frame from the GIF.
     */
    void UpdateFrame(const QImage &frame) const;

    /**
     * @brief Slot connected to the decoder's positionChanged signal.
     * Updates the internal current_position_ variable. (Currently not used elsewhere).
     * @param position The current playback position in seconds.
     */
    void UpdatePosition(const double position) {
        current_position_ = position;
    }

    /**
     * @brief Slot connected to the decoder's error signal.
     * Logs the error message and displays it in the QLabel. Sets a minimum size for the widget.
     * @param message The error message from the decoder.
     */
    void HandleError(const QString &message);

    /**
     * @brief Slot connected to the decoder's gifInfo signal.
     * Stores the GIF's dimensions, duration, and frame rate. Updates the widget's geometry
     * based on the dimensions and emits the gifLoaded() signal. Redisplays the thumbnail if needed.
     * @param width The width of the GIF in pixels.
     * @param height The height of the GIF in pixels.
     * @param duration The total duration of the GIF in seconds.
     * @param frame_rate The calculated average frame rate.
     */
    void HandleGifInfo(int width, int height, double duration, double frame_rate);

signals:
    /**
     * @brief Emitted after the GIF information (dimensions, duration, etc.) has been successfully received from the decoder.
     * Can be used by parent widgets to adjust layout once the GIF size is known.
     */
    void gifLoaded();

private:
    /** @brief QLabel used to display the GIF frames. ScaledContents is enabled. */
    QLabel *gif_label_;
    /** @brief Shared pointer to the GifDecoder instance responsible for decoding. */
    std::shared_ptr<GifDecoder> decoder_;

    /** @brief Stores the raw GIF data passed in the constructor. */
    QByteArray gif_data_;

    /** @brief Original width of the GIF in pixels. Set by HandleGifInfo. */
    int gif_width_ = 0;
    /** @brief Original height of the GIF in pixels. Set by HandleGifInfo. */
    int gif_height_ = 0;
    /** @brief Total duration of the GIF in seconds. Set by HandleGifInfo. */
    double gif_duration_ = 0;
    /** @brief Average frame rate of the GIF. Set by HandleGifInfo. */
    double frame_rate_ = 0;
    /** @brief Current playback position in seconds. Updated by UpdatePosition. */
    double current_position_ = 0;
    /** @brief Stores the first frame as a QImage (potentially unused after thumbnail_pixmap_ is set). */
    QImage thumbnail_frame_;
    /** @brief Flag indicating if the GIF is currently playing (mouse hover). */
    bool is_playing_ = false;
    /** @brief Stores the first frame as a QPixmap for efficient display as a thumbnail. */
    QPixmap thumbnail_pixmap_;
    /** @brief Pointer to the translation manager for handling UI translations. */
    TranslationManager *translator_ = nullptr;
};

#endif //INLINE_GIF_PLAYER_H
