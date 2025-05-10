#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4267 4996)
#endif

#include <QBuffer>

#include "../../audio/decoder/audio_decoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif

/**
 * @brief Decodes video data frame by frame using FFmpeg libraries in a separate thread.
 *
 * This class takes raw video data (e.g., MP4, WebM) as a QByteArray, uses FFmpeg
 * (libavformat, libavcodec, libswscale) to decode the video stream, and emits each
 * decoded frame as a QImage (Format_RGB888). If an audio stream is present, it
 * uses an internal AudioDecoder instance to handle audio decoding and playback
 * synchronization.
 *
 * The decoding loop runs in a separate QThread to avoid blocking the main UI thread.
 * It supports pausing, resuming, seeking, volume control (via the internal AudioDecoder),
 * and provides signals for status updates (errors, info, position, frames, finish).
 * It uses a custom AVIOContext to read data directly from the QByteArray in memory.
 */
class VideoDecoder final : public QThread {
    Q_OBJECT

public:
    /**
     * @brief Constructs a VideoDecoder object.
     * Initializes internal state, allocates memory for FFmpeg's custom I/O context,
     * sets up the QBuffer for reading from the provided video data.
     * @param video_data The raw video data to be decoded.
     * @param parent Optional parent QObject.
     */
    explicit VideoDecoder(const QByteArray &video_data, QObject *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops the decoding thread, waits for it to finish, and releases all allocated resources.
     */
    ~VideoDecoder() override;

    /**
     * @brief Initializes the FFmpeg components for decoding the video and potentially audio.
     * Opens the input stream using the custom I/O context, finds video and audio streams,
     * opens codecs, sets up the video scaler (to convert to RGB24), allocates frames/buffers.
     * If audio is found, creates and connects to an internal AudioDecoder instance.
     * Emits videoInfo upon success.
     * This operation is thread-safe and should be called before starting the thread.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Synchronously extracts the first video frame from the data.
     * Performs a separate, simplified FFmpeg initialization and decoding process
     * to get the first frame quickly, typically for generating a thumbnail.
     * Emits frameReady() with the first frame upon success.
     * This method runs in the calling thread.
     */
    void ExtractFirstFrame();

    /**
     * @brief Releases all FFmpeg and internal AudioDecoder resources.
     * Calls CleanupFFmpegResources(). This operation is thread-safe.
     */
    void ReleaseResources() {
        CleanupFFmpegResources();
    }

    /**
     * @brief Resets the playback position to the beginning of the video.
     * Pauses playback, seeks both video and internal audio decoder (if present) to 0,
     * resets internal state flags (finished, first frame), and wakes the thread.
     * This operation is thread-safe.
     */
    void Reset();

    /**
     * @brief Stops the decoding thread and playback gracefully.
     * Sets the stopped_ flag, stops the internal audio decoder (if present),
     * and wakes the thread if it's waiting. The thread will then exit its run loop.
     * This operation is thread-safe.
     */
    void Stop();

    /**
     * @brief Toggles the paused state of the playback.
     * If paused, playback stops; if unpaused, playback resumes.
     * Also pauses/resumes the internal audio decoder (if present).
     * Wakes the thread if it was waiting due to being paused. This operation is thread-safe.
     */
    void Pause();

    /**
     * @brief Seeks to a specific position in the video stream.
     * Seeks the internal audio decoder (if present) and sets the seeking flag and target
     * timestamp for the video stream. The actual video seek operation happens within the run() loop.
     * This operation is thread-safe.
     * @param position The target position in seconds from the beginning of the video.
     */
    void Seek(double position);

    /**
     * @brief Sets the playback volume (delegated to the internal AudioDecoder).
     * Has no effect if there is no audio stream.
     * This operation is thread-safe.
     * @param volume The desired volume level (0.0 to 1.0).
     */
    void SetVolume(float volume) const {
        if (audio_decoder_) {
            audio_decoder_->SetVolume(volume);
        }
    }

    /**
     * @brief Gets the current playback volume (from the internal AudioDecoder).
     * Returns 0.0 if there is no audio stream.
     * This operation is thread-safe.
     * @return The current volume level (0.0 to 1.0).
     */
    float GetVolume() const {
        return audio_decoder_ ? audio_decoder_->GetVolume() : 0.0f;
    }

    /**
     * @brief Checks if the playback is currently paused.
     * This operation is thread-safe.
     * @return True if paused, false otherwise.
     */
    bool IsPaused() const {
        QMutexLocker locker(&mutex_);
        return paused_;
    }

    /**
     * @brief Checks if the video file contains an audio stream.
     * @return True if an audio stream was detected during initialization, false otherwise.
     */
    bool HasAudio() const {
        // Check if audio_stream_index_ was successfully found during initialization
        return audio_stream_index_ != -1;
    }

signals:
    /**
     * @brief Emitted for each decoded video frame.
     * @param frame The decoded frame as a QImage (Format_RGB888). A copy is emitted.
     */
    void frameReady(const QImage &frame);

    /**
     * @brief Emitted after successful initialization, providing video stream details.
     * @param width The width of the video in pixels.
     * @param height The height of the video in pixels.
     * @param fps The calculated average frame rate in frames per second.
     * @param duration The total duration of the video in seconds.
     * @param has_audio True if an audio stream was detected, false otherwise.
     */
    void videoInfo(int width, int height, double fps, double duration, bool has_audio);

    /**
     * @brief Emitted periodically during playback to indicate the current position (based on audio if available, otherwise estimated).
     * @param position The current playback position in seconds.
     */
    void positionChanged(double position);

    /**
     * @brief Emitted when both video and audio (if present) streams reach the end.
     */
    void playbackFinished();

    /**
     * @brief Emitted when an error occurs during initialization or decoding.
     * @param message A description of the error.
     */
    void error(const QString &message);

protected:
    /**
     * @brief The main function executed by the QThread.
     * Contains the loop that reads packets, decodes video frames, handles seeking/pausing,
     * synchronizes with the internal audio decoder (if present) or uses frame timing,
     * and emits decoded frames via the frameReady signal.
     */
    void run() override;

private slots:
    /**
     * @brief Slot connected to the internal AudioDecoder's error signal.
     * Relays the audio error message via the main error() signal.
     * @param message The error message from the AudioDecoder.
     */
    void HandleAudioError(const QString &message) {
        emit error("Błąd strumienia audio: " + message);
    }

    /**
     * @brief Slot connected to the internal AudioDecoder's playbackFinished signal.
     * Sets the audio_finished_ flag and emits the main playbackFinished() signal
     * if the video stream has also finished.
     */
    void HandleAudioFinished();

    /**
     * @brief Slot connected to the internal AudioDecoder's positionChanged signal.
     * Updates the internal current_audio_position_ and emits the main positionChanged() signal.
     * @param position The current audio playback position in seconds.
     */
    void UpdateAudioPosition(double position);

private:
    /**
     * @brief Releases all FFmpeg resources (contexts, frames, scaler) and deletes the internal AudioDecoder.
     * Resets internal state variables related to FFmpeg and audio.
     */
    void CleanupFFmpegResources();

    /**
     * @brief Custom read function for FFmpeg's AVIOContext.
     * Reads data from the internal QBuffer (buffer_).
     * @param opaque Pointer to the QBuffer instance.
     * @param buf Buffer to read data into.
     * @param buf_size Size of the buffer.
     * @return Number of bytes read, or 0 if the end of buffer is reached.
     */
    static int ReadPacket(void *opaque, uint8_t *buf, int buf_size);

    /**
     * @brief Custom seek function for FFmpeg's AVIOContext.
     * Adjusts the position within the internal QBuffer (buffer_).
     * Supports SEEK_SET and AVSEEK_SIZE.
     * @param opaque Pointer to the QBuffer instance.
     * @param offset The offset to seek to/by.
     * @param whence The seeking mode (SEEK_SET, AVSEEK_SIZE).
     * @return The new buffer position, or the total size if whence is AVSEEK_SIZE, or -1 on error.
     */
    static int64_t SeekPacket(void *opaque, int64_t offset, int whence);

    /** @brief The raw video data provided in the constructor. */
    QByteArray video_data_;
    /** @brief QBuffer used by FFmpeg's custom I/O context to read video_data_. */
    QBuffer buffer_;
    /** @brief FFmpeg context for custom I/O operations (reading from memory). */
    AVIOContext *io_context_ = nullptr;
    /** @brief FFmpeg context for handling the container format (e.g., MP4, WebM). */
    AVFormatContext *format_context_ = nullptr;
    /** @brief FFmpeg context for the video codec. */
    AVCodecContext *codec_context_ = nullptr;
    /** @brief FFmpeg context for image scaling and pixel format conversion (to RGB24). */
    SwsContext *sws_context_ = nullptr;
    /** @brief FFmpeg frame structure to hold decoded video data. */
    AVFrame *frame_ = nullptr;
    /** @brief Index of the video stream within the format context. */
    int video_stream_ = -1;
    /** @brief Calculated average frame rate of the video in frames per second. */
    double frame_rate_ = 0.0;
    /** @brief Total duration of the video in seconds. */
    double duration_ = 0.0;
    /** @brief Current estimated playback position in seconds (used if no audio). */
    double current_position_ = 0.0;

    /** @brief Mutex protecting access to shared state variables (paused_, stopped_, seeking_, etc.). */
    mutable QMutex mutex_;
    /** @brief Condition variable used to pause/resume the decoding thread. */
    QWaitCondition wait_condition_;
    /** @brief Flag indicating if the thread should stop execution. Access protected by mutex_. */
    bool stopped_ = false;
    /** @brief Flag indicating if playback is currently paused. Access protected by mutex_. */
    bool paused_ = false;
    /** @brief Flag indicating if a seek operation is pending. Access protected by mutex_. */
    bool seeking_ = false;
    /** @brief Target timestamp (in video stream timebase) for the pending seek operation. Access protected by mutex_. */
    int64_t seek_position_ = -1;

    /** @brief Timer used for frame timing synchronization when no audio is present. */
    QElapsedTimer frame_timer_;
    /** @brief Flag indicating if the current frame is the first one after start/seek. */
    bool first_frame_ = true;
    /** @brief Presentation timestamp (PTS) of the previously decoded frame (used for frame timing). */
    int64_t last_frame_pts_ = AV_NOPTS_VALUE;

    /** @brief Pointer to the internal AudioDecoder instance (if audio stream exists). */
    AudioDecoder *audio_decoder_ = nullptr;
    /** @brief Index of the audio stream within the format context. */
    int audio_stream_index_ = -1;
    /** @brief Current playback position reported by the internal AudioDecoder. Access protected by mutex_. */
    double current_audio_position_ = 0.0;
    /** @brief Flag indicating if the internal AudioDecoder has been successfully initialized. */
    bool audio_initialized_ = false;
    /** @brief Flag indicating if the internal AudioDecoder has finished playback. Access protected by mutex_. */
    bool audio_finished_ = false;
    /** @brief Flag indicating if the video stream has reached the end. Access protected by mutex_. */
    bool video_finished_ = false;
};

#endif // VIDEO_DECODER_H
