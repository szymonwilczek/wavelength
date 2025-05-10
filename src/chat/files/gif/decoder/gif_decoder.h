#ifndef GIF_DECODER_H
#define GIF_DECODER_H

#ifdef _MSC_VER
#pragma comment(lib, "swscale.lib")
#endif

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QElapsedTimer>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

/**
 * @brief Decodes GIF data frame by frame using FFmpeg libraries in a separate thread.
 *
 * This class takes raw GIF data as a QByteArray, uses FFmpeg (libavformat, libavcodec, libswscale)
 * to decode it frame by frame, and emits each decoded frame as a QImage (Format_RGBA8888).
 * It runs the decoding loop in a separate QThread to avoid blocking the main UI thread.
 * It supports pausing, resuming, looping, and provides signals
 * for status updates (errors, info, position, frames). It uses a custom AVIOContext to read
 * data directly from the QByteArray in memory.
 */
class GifDecoder final : public QThread {
    Q_OBJECT

public:
    /**
     * @brief Constructs a GifDecoder object.
     * Initializes internal state, allocates memory for FFmpeg's custom I/O context,
     * and copies the provided GIF data into an internal buffer.
     * @param gif_data The raw GIF data to be decoded.
     * @param parent Optional parent QObject.
     */
    explicit GifDecoder(const QByteArray &gif_data, QObject *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops the decoding thread, waits for it to finish, and releases all allocated resources.
     */
    ~GifDecoder() override;

    /**
     * @brief Releases all FFmpeg resources (contexts, frames, buffers).
     * Does not release the custom I/O context buffer, as that's handled separately.
     * Resets the initialization flag. This operation is thread-safe.
     */
    void ReleaseResources();

    /**
     * @brief Reinitializes the decoder after it has been stopped or encountered an error.
     * Stops the thread if running, releases existing FFmpeg resources, resets state flags
     * (paused, stopped, position), and reallocates I/O context if necessary.
     * Does not automatically start playback. This operation is thread-safe.
     * @return True if reinitialization setup was successful, false otherwise.
     */
    bool Reinitialize();

    /**
     * @brief Initializes the FFmpeg components for decoding the GIF.
     * Opens the input stream using the custom I/O context, finds the GIF stream and codec,
     * opens the codec, sets up the scaler (to convert to RGBA), allocates frames and buffers.
     * Emits gifInfo and firstFrameReady upon success.
     * This operation is thread-safe and should be called before starting the thread.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Stops the decoding thread gracefully.
     * Sets the stopped_ flag, unpauses if necessary, and wakes the thread if it's waiting.
     * The thread will then exit its run loop. This operation is thread-safe.
     */
    void Stop();

    /**
     * @brief Pauses the decoding loop.
     * Sets the paused_ flag. The thread will wait on the wait_condition_ in its next iteration.
     * This operation is thread-safe.
     */
    void Pause();

    /**
     * @brief Checks if the decoding is currently paused.
     * This operation is thread-safe.
     * @return True if paused, false otherwise.
     */
    bool IsPaused() const {
        QMutexLocker locker(&mutex_); // Ensure thread safety when reading paused_
        return paused_;
    }

    /**
     * @brief Resets the playback position to the beginning of the GIF.
     * Seeks the FFmpeg format context to the start, flushes codec buffers, resets the internal position counter,
     * and emits positionChanged(0). This operation is thread-safe.
     */
    void Reset();

    /**
     * @brief Checks if the decoding thread is currently running.
     * @return True if the thread is running, false otherwise.
     */
    bool IsDecoderRunning() const {
        return isRunning();
    }

    /**
     * @brief Gets the total duration of the GIF in seconds, if available.
     * Reads the duration from the FFmpeg format context.
     * @return The duration in seconds, or 0.0 if not available.
     */
    double GetDuration() const {
        QMutexLocker locker(&mutex_); // Protect access to format_context_
        if (format_context_ && format_context_->duration != AV_NOPTS_VALUE) {
            return format_context_->duration / static_cast<double>(AV_TIME_BASE);
        }
        return 0.0;
    }

    /**
     * @brief Resumes decoding if paused or starts the thread if not already running.
     * Clears the paused_ flag and wakes the thread if it's waiting. If the thread
     * hasn't been started yet, it calls start(). This operation is thread-safe.
     */
    void Resume();

signals:
    /**
     * @brief Emitted for each decoded frame of the GIF.
     * @param frame The decoded frame as a QImage (Format_RGBA8888). A copy is emitted.
     */
    void frameReady(const QImage &frame);

    /**
     * @brief Emitted once after successful initialization, containing the first frame.
     * Useful for displaying a static preview before the animation starts.
     * @param frame The first decoded frame as a QImage (Format_RGBA8888). A copy is emitted.
     */
    void firstFrameReady(const QImage &frame);

    /**
     * @brief Emitted when an error occurs during initialization or decoding.
     * @param message A description of the error.
     */
    void error(const QString &message);

    /**
     * @brief Emitted after successful initialization, providing GIF stream details.
     * @param width The width of the GIF in pixels.
     * @param height The height of the GIF in pixels.
     * @param duration The total duration of the GIF in seconds (can be approximate).
     * @param frame_rate The calculated average frame rate in frames per second.
     * @param num_of_streams The number of streams found (should typically be 1 for GIF).
     */
    void gifInfo(int width, int height, double duration, double frame_rate, int num_of_streams);

    /**
     * @brief Emitted when the decoder reaches the end of the stream (before looping).
     * Note: Currently not emitted as the decoder loops indefinitely.
     */
    void playbackFinished();

    /**
     * @brief Emitted periodically during playback to indicate the current position.
     * @param position The current playback position in seconds.
     */
    void positionChanged(double position);

protected:
    /**
     * @brief The main function executed by the QThread.
     * Contains the loop that reads packets, decodes frames, handles pausing/looping,
     * calculates frame delays, and emits decoded frames via the frameReady signal.
     */
    void run() override;

private:
    /**
     * @brief Custom read function for FFmpeg's AVIOContext.
     * Reads data from the internal gif_data_ QByteArray buffer.
     * @param opaque Pointer to the GifDecoder instance.
     * @param buf Buffer to read data into.
     * @param buf_size Size of the buffer.
     * @return Number of bytes read, or AVERROR_EOF if the end of data is reached.
     */
    static int ReadPacket(void *opaque, uint8_t *buf, int buf_size);

    /**
     * @brief Custom seek function for FFmpeg's AVIOContext.
     * Adjusts the internal read_position_ based on the offset and whence parameters.
     * Supports SEEK_SET, SEEK_CUR, SEEK_END, and AVSEEK_SIZE.
     * @param opaque Pointer to the GifDecoder instance.
     * @param offset The offset to seek to/by.
     * @param whence The seeking mode (SEEK_SET, SEEK_CUR, SEEK_END, AVSEEK_SIZE).
     * @return The new read position, or the total size if whence is AVSEEK_SIZE, or -1 on error.
     */
    static int64_t SeekPacket(void *opaque, int64_t offset, int whence);

    /**
     * @brief Internal helper function called by Initialize() to extract and emit the first frame.
     * Seeks to the beginning, decodes frames until the first one is successfully obtained,
     * emits it via firstFrameReady, and then seeks back to the beginning for the main run() loop.
     */
    void ExtractAndEmitFirstFrameInternal();

    /** @brief The raw GIF data provided in the constructor. */
    QByteArray gif_data_;
    /** @brief Current read position within gif_data_ for the custom I/O context. */
    int read_position_ = 0;

    /** @brief FFmpeg context for handling the container format (GIF). */
    AVFormatContext *format_context_ = nullptr;
    /** @brief FFmpeg context for the GIF codec. */
    AVCodecContext *codec_context_ = nullptr;
    /** @brief FFmpeg context for image scaling and pixel format conversion (to RGBA). */
    SwsContext *sws_context_ = nullptr;
    /** @brief FFmpeg frame structure to hold the raw decoded frame data. */
    AVFrame *frame_ = nullptr;
    /** @brief FFmpeg frame structure to hold the frame data after conversion to RGBA. */
    AVFrame *frame_rgb_ = nullptr;
    /** @brief FFmpeg context for custom I/O operations (reading from memory). */
    AVIOContext *io_context_ = nullptr;
    /** @brief Buffer used by the custom I/O context to hold gif_data_. */
    unsigned char *io_buffer_ = nullptr;
    /** @brief Index of the video stream within the format context. */
    int gif_stream_ = -1;
    /** @brief Buffer holding the pixel data for frame_rgb_. */
    uint8_t *buffer_ = nullptr;
    /** @brief Size of the buffer_ in bytes. */
    int buffer_size_ = 0;

    /** @brief Mutex protecting access to shared state variables from different threads. */
    mutable QMutex mutex_; // Marked mutable to allow locking in const methods like GetDuration/IsPaused
    /** @brief Condition variable used to pause/resume the decoding thread. */
    QWaitCondition wait_condition_;
    /** @brief Flag indicating if the thread should stop execution. Access protected by mutex_. */
    bool stopped_ = false;
    /** @brief Flag indicating if playback is currently paused. Access protected by mutex_. */
    bool paused_ = true;
    /** @brief Target timestamp for the pending seek operation (in stream timebase). Access protected by mutex_. */
    int64_t seek_position_ = 0;
    /** @brief Current playback position in seconds. Access protected by mutex_. */
    double current_position_ = 0;
    /** @brief Flag indicating if the end of the GIF stream has been reached (before looping). Access protected by mutex_. */
    bool reached_end_of_stream_ = false;
    /** @brief Flag indicating if the Initialize() method has completed successfully. Access protected by mutex_. */
    bool initialized_ = false;

    /** @brief Calculated average frame rate of the GIF in frames per second. */
    double frame_rate_ = 0.0;
    /** @brief Calculated delay between frames in milliseconds. */
    double frame_delay_ = 100.0; // ms
    /** @brief Timer used to help synchronize frame emission based on frame_delay_. */
    QElapsedTimer frame_timer_;
};

#endif // GIF_DECODER_H
