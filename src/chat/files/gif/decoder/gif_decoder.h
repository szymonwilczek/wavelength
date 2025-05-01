#ifndef GIF_DECODER_H
#define GIF_DECODER_H

#ifdef _MSC_VER
#pragma comment(lib, "swscale.lib")
#endif

#include <QImage>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QDebug>
#include <QElapsedTimer>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class GifDecoder final : public QThread {
    Q_OBJECT

public:
    explicit GifDecoder(const QByteArray& gif_data, QObject* parent = nullptr);

    ~GifDecoder() override;

    void ReleaseResources();

    bool Reinitialize();

    bool Initialize();

    void Stop();

    void Pause();

    bool IsPaused() const {
        return paused_;
    }

    void Seek(double position);

    void Reset();

    bool IsDecoderRunning() const {
        return isRunning();
    }

    double GetDuration() const {
        if (format_context_ && format_context_->duration != AV_NOPTS_VALUE) {
            return format_context_->duration / static_cast<double>(AV_TIME_BASE);
        }
        return 0.0;
    }

    void Resume();

signals:
    void frameReady(const QImage& frame);
    void firstFrameReady(const QImage& frame);
    void error(const QString& message);
    void gifInfo(int width, int height, double duration, double frame_rate, int num_of_streams);
    void playbackFinished();
    void positionChanged(double position);

protected:
     void run() override;

private:
    static int ReadPacket(void* opaque, uint8_t* buf, int buf_size);

    static int64_t SeekPacket(void* opaque, int64_t offset, int whence);

    void ExtractAndEmitFirstFrameInternal();

    QByteArray gif_data_;
    int read_position_ = 0;

    AVFormatContext* format_context_ = nullptr;
    AVCodecContext* codec_context_ = nullptr;
    SwsContext* sws_context_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVFrame* frame_rgb_ = nullptr;
    AVIOContext* io_context_ = nullptr;
    unsigned char* io_buffer_ = nullptr;
    int gif_stream_ = -1;
    uint8_t* buffer_ = nullptr;
    int buffer_size_ = 0;

    QMutex mutex_;
    QWaitCondition wait_condition_;
    bool stopped_ = false;
    bool paused_ = true;
    bool seeking_ = false;
    int64_t seek_position_ = 0;
    double current_position_ = 0;
    bool reached_end_of_stream_ = false;
    bool initialized_ = false;

    double frame_rate_ = 0.0;
    double frame_delay_ = 100.0; // ms
    QElapsedTimer frame_timer_;
};

#endif // GIF_DECODER_H