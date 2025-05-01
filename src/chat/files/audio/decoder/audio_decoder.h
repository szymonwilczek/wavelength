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

class AudioDecoder final : public QThread {
    Q_OBJECT

public:
    explicit AudioDecoder(const QByteArray& audio_data, QObject* parent = nullptr);

    ~AudioDecoder() override;

    void ReleaseResources();

    bool Reinitialize();

    void SetVolume(float volume) const;

    float GetVolume() const;

    bool Initialize();

    void Stop();

    void Pause();

    bool IsPaused() const;

    void Seek(double position);

    void Reset();

    bool IsDecoderRunning() const {
        return isRunning();
    }

signals:
    void error(const QString& message);
    void audioInfo(int sample_rate, int channels, double duration);
    void playbackFinished();
    void positionChanged(double position);

protected:
    void run() override;

    void DecodeAudioFrame(const AVFrame* audio_frame) const;

private:
    static int ReadPacket(void* opaque, uint8_t* buf, int buf_size);

    static int64_t SeekPacket(void* opaque, int64_t offset, int whence);

    QByteArray audio_data_;
    int read_position_ = 0;

    AVFormatContext* format_context_;
    int audio_stream_ = -1;
    AVCodecContext* audio_codec_context_ = nullptr;
    SwrContext* swr_context_ = nullptr;
    AVFrame* audio_frame_ = nullptr;
    AVIOContext* io_context_ = nullptr;
    unsigned char* io_buffer_ = nullptr;

    QAudioOutput* audio_output_ = nullptr;
    QIODevice* audio_device_ = nullptr;
    QAudioFormat audio_format_;

    mutable QMutex mutex_;
    QWaitCondition wait_condition_;
    bool stopped_;
    bool paused_ = true;
    bool seeking_ = false;
    int64_t seek_position_ = 0;
    double current_position_ = 0;
    bool reached_end_of_stream_ = false;
    bool initialized_ = false;
};

#endif // AUDIO_DECODER_H