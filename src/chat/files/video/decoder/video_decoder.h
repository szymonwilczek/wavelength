#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4267 4996)
#endif

#include <QImage>
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

class VideoDecoder final : public QThread {
    Q_OBJECT

public:
    explicit VideoDecoder(const QByteArray& video_data, QObject* parent = nullptr);

    ~VideoDecoder() override;

    bool Initialize();

    void ExtractFirstFrame();

    void ReleaseResources() {
        CleanupFFmpegResources();
    }

    void Reset();

    void Stop();

    void Pause();

    void Seek(double position);

    void SetVolume(const float volume) const {
        if (audio_decoder_) {
            audio_decoder_->SetVolume(volume);
        }
    }

    float GetVolume() const {
        return audio_decoder_ ? audio_decoder_->GetVolume() : 0.0f;
    }

    bool IsPaused() const {
        QMutexLocker locker(&mutex_);
        return paused_;
    }

    bool HasAudio() const {
        return audio_decoder_ != nullptr;
    }

signals:
    void frameReady(const QImage& frame);
    void videoInfo(int width, int height, double fps, double duration, bool has_audio);
    void positionChanged(double position);
    void playbackFinished();
    void error(const QString& message);

protected:
    void run() override;

private slots:
    void HandleAudioError(const QString& message) {
        emit error("Błąd strumienia audio: " + message);
    }

    void HandleAudioFinished();

    void UpdateAudioPosition(double position);

private:
    void CleanupFFmpegResources();

    static int ReadPacket(void* opaque, uint8_t* buf, int buf_size);

    static int64_t SeekPacket(void* opaque, int64_t offset, int whence);

    QByteArray video_data_;
    QBuffer buffer_;
    AVIOContext* io_context_ = nullptr;
    AVFormatContext* format_context_ = nullptr;
    AVCodecContext* codec_context_ = nullptr;
    SwsContext* sws_context_ = nullptr;
    AVFrame* frame_ = nullptr;
    int video_stream_ = -1;
    double frame_rate_ = 0.0;
    double duration_ = 0.0;
    double current_position_ = 0.0;

    mutable QMutex mutex_;
    QWaitCondition wait_condition_;
    bool stopped_ = false;
    bool paused_ = false;
    bool seeking_ = false;
    int64_t seek_position_ = -1;

    QElapsedTimer frame_timer_;
    bool first_frame_ = true;
    int64_t last_frame_pts_ = AV_NOPTS_VALUE;

    AudioDecoder* audio_decoder_ = nullptr;
    int audio_stream_index_ = -1;
    double current_audio_position_ = 0.0;
    bool audio_initialized_ = false;
    bool audio_finished_ = false;
    bool video_finished_ = false;
};

#endif // VIDEO_DECODER_H