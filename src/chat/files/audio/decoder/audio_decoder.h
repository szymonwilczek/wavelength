#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#ifdef _MSC_VER
#pragma comment(lib, "swresample.lib")
#endif

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QIODevice>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

/**
 * @brief Decodes and plays audio data using FFmpeg libraries in a separate thread.
 *
 * This class takes raw audio data (e.g., from a file) as a QByteArray,
 * uses FFmpeg (libavformat, libavcodec, libswresample) to decode it,
 * and plays the resulting PCM audio using Qt Multimedia (QAudioOutput).
 * It runs the decoding and playback loop in a separate QThread to avoid
 * blocking the main UI thread. It supports pausing, seeking, volume control,
 * and provides signals for status updates (errors, position, finish).
 */
class AudioDecoder final : public QThread {
    Q_OBJECT

public:
    /**
     * @brief Constructs an AudioDecoder object.
     * Initializes internal state, allocates memory for FFmpeg's custom I/O context,
     * and copies the provided audio data into an internal buffer.
     * @param audio_data The raw audio data to be decoded.
     * @param parent Optional parent QObject.
     */
    explicit AudioDecoder(const QByteArray &audio_data, QObject *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops the decoding thread, waits for it to finish, and releases all allocated resources.
     */
    ~AudioDecoder() override;

    /**
     * @brief Releases all FFmpeg and Qt Multimedia resources.
     * Stops audio output, frees contexts (format, codec, resampler, I/O), frames, and buffers.
     * Resets the initialization flag. This operation is thread-safe.
     */
    void ReleaseResources();

    /**
     * @brief Reinitializes the decoder after it has been stopped or encountered an error.
     * Releases existing resources, resets state flags (paused, stopped, seeking, position),
     * reallocates I/O buffer and context if necessary. Does not automatically start playback.
     * This operation is thread-safe.
     * @return True if reinitialization setup was successful, false otherwise.
     */
    bool Reinitialize();

    /**
     * @brief Sets the playback volume.
     * The volume is a linear factor between 0.0 (silent) and 1.0 (full volume).
     * This operation is thread-safe.
     * @param volume The desired volume level (0.0 to 1.0).
     */
    void SetVolume(float volume) const;

    /**
     * @brief Gets the current playback volume.
     * This operation is thread-safe.
     * @return The current volume level (0.0 to 1.0).
     */
    float GetVolume() const;

    /**
     * @brief Initializes the FFmpeg components and QAudioOutput for playback.
     * Opens the input stream using the custom I/O context, finds the audio stream and codec,
     * opens the codec, sets up the resampler (to convert to PCM S16LE stereo 44.1 kHz),
     * configures and starts QAudioOutput. Emits audioInfo upon success.
     * This operation is thread-safe and should be called before starting the thread.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Stops the decoding thread and playback gracefully.
     * Sets the stopped_ flag and wakes the thread if it's waiting.
     * The thread will then exit its run loop. This operation is thread-safe.
     */
    void Stop();

    /**
     * @brief Toggles the paused state of the playback.
     * If paused, playback stops; if unpaused, playback resumes.
     * Wakes the thread if it was waiting due to being paused. This operation is thread-safe.
     */
    void Pause();

    /**
     * @brief Checks if the playback is currently paused.
     * This operation is thread-safe.
     * @return True if paused, false otherwise.
     */
    bool IsPaused() const;

    /**
     * @brief Seeks to a specific position in the audio stream.
     * The position is specified in seconds. Sets the seeking_ flag and wakes the thread.
     * The actual seek operation happens within the run() loop.
     * This operation is thread-safe.
     * @param position The target position in seconds from the beginning of the audio.
     */
    void Seek(double position);

    /**
     * @brief Resets the playback position to the beginning (0 seconds).
     * Seeks the FFmpeg format context, flushes codec buffers, resets the internal position counter,
     * resets the audio output device, and emits positionChanged(0).
     * This operation is thread-safe.
     */
    void Reset();

    /**
     * @brief Checks if the decoding thread is currently running.
     * @return True if the thread is running, false otherwise.
     */
    bool IsDecoderRunning() const {
        return isRunning();
    }

signals:
    /**
     * @brief Emitted when an error occurs during initialization or decoding.
     * @param message A description of the error.
     */
    void error(const QString &message);

    /**
     * @brief Emitted after successful initialization, providing audio stream details.
     * @param sample_rate The sample rate of the audio in Hz.
     * @param channels The number of audio channels.
     * @param duration The total duration of the audio in seconds.
     */
    void audioInfo(int sample_rate, int channels, double duration);

    /**
     * @brief Emitted when the decoder reaches the end of the audio stream.
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
     * Contains the loop that reads packets, decodes frames, handles seeking/pausing,
     * updates playback position, and sends decoded audio data to the output device.
     */
    void run() override;

    /**
     * @brief Decodes a single audio frame using the FFmpeg resampler and writes the result to the audio device.
     * Converts the input frame (audio_frame) to the target format (PCM S16LE stereo 44.1 kHz),
     * allocates a buffer for the converted data, performs the conversion using swr_convert,
     * and writes the resulting data to the QAudioOutput device (audio_device_).
     * Includes logic to wait if the audio device buffer is nearly full.
     * This method is thread-safe regarding access to shared decoder state via mutex_.
     * @param audio_frame The raw audio frame received from the FFmpeg decoder.
     */
    void DecodeAudioFrame(const AVFrame *audio_frame) const;

private:
    /**
     * @brief Custom read function for FFmpeg's AVIOContext.
     * Reads data from the internal audio_data_ QByteArray buffer.
     * @param opaque Pointer to the AudioDecoder instance.
     * @param buf Buffer to read data into.
     * @param buf_size Size of the buffer.
     * @return Number of bytes read, or AVERROR_EOF if the end of data is reached.
     */
    static int ReadPacket(void *opaque, uint8_t *buf, int buf_size);

    /**
     * @brief Custom seek function for FFmpeg's AVIOContext.
     * Adjusts the internal read_position_ based on the offset and whence parameters.
     * Supports SEEK_SET, SEEK_CUR, SEEK_END, and AVSEEK_SIZE.
     * @param opaque Pointer to the AudioDecoder instance.
     * @param offset The offset to seek to/by.
     * @param whence The seeking mode (SEEK_SET, SEEK_CUR, SEEK_END, AVSEEK_SIZE).
     * @return The new read position, or the total size if whence is AVSEEK_SIZE, or -1 on error.
     */
    static int64_t SeekPacket(void *opaque, int64_t offset, int whence);

    /** @brief The raw audio data provided in the constructor. */
    QByteArray audio_data_;
    /** @brief Current read position within audio_data_ for the custom I/O context. */
    int read_position_ = 0;

    /** @brief FFmpeg context for handling the container format. */
    AVFormatContext *format_context_ = nullptr;
    /** @brief Index of the audio stream within the format context. */
    int audio_stream_ = -1;
    /** @brief FFmpeg context for the audio codec. */
    AVCodecContext *audio_codec_context_ = nullptr;
    /** @brief FFmpeg context for audio resampling (format/rate/channel conversion). */
    SwrContext *swr_context_ = nullptr;
    /** @brief FFmpeg frame structure to hold decoded audio data. */
    AVFrame *audio_frame_ = nullptr;
    /** @brief FFmpeg context for custom I/O operations (reading from memory). */
    AVIOContext *io_context_ = nullptr;
    /** @brief Buffer used by the custom I/O context to hold audio_data_. */
    unsigned char *io_buffer_ = nullptr;

    /** @brief Qt Multimedia object for audio output. */
    QAudioOutput *audio_output_ = nullptr;
    /** @brief Qt I/O device associated with audio_output_, used for writing PCM data. */
    QIODevice *audio_device_ = nullptr;
    /** @brief Qt audio format description (PCM S16LE stereo 44.1kHz). */
    QAudioFormat audio_format_;

    /** @brief Mutex protecting access to shared state variables from different threads. */
    mutable QMutex mutex_;
    /** @brief Condition variable used to pause/resume the decoding thread. */
    QWaitCondition wait_condition_;
    /** @brief Flag indicating if the thread should stop execution. */
    bool stopped_ = false;
    /** @brief Flag indicating if playback is currently paused. */
    bool paused_ = true;
    /** @brief Flag indicating if a seek operation is pending. */
    bool seeking_ = false;
    /** @brief Target timestamp for the pending seek operation (in stream timebase). */
    int64_t seek_position_ = 0;
    /** @brief Current playback position in seconds. */
    double current_position_ = 0;
    /** @brief Flag indicating if the end of the audio stream has been reached. */
    bool reached_end_of_stream_ = false;
    /** @brief Flag indicating if the Initialize() method has completed successfully. */
    bool initialized_ = false;
};

#endif // AUDIO_DECODER_H
