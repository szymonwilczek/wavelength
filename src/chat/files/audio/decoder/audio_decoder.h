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

class AudioDecoder : public QThread {
    Q_OBJECT

public:
    AudioDecoder(const QByteArray& audioData, QObject* parent = nullptr);

    ~AudioDecoder();

    void releaseResources();

    bool reinitialize();

    void setVolume(float volume);

    float getVolume() const;

    bool initialize();

    void stop();

    void pause();

    bool isPaused() const;

    void seek(double position);

    void reset();

    bool isRunning() const {
        return QThread::isRunning();
    }

signals:
    void error(const QString& message);
    void audioInfo(int sampleRate, int channels, double duration);
    void playbackFinished();
    void positionChanged(double position);

protected:
    void run() override;

    void decodeAudioFrame(AVFrame* audioFrame);

private:
    // Funkcje callback dla custom I/O
    static int readPacket(void* opaque, uint8_t* buf, int buf_size);

    static int64_t seekPacket(void* opaque, int64_t offset, int whence);

    QByteArray m_audioData;
    int m_readPosition = 0;

    AVFormatContext* m_formatContext;
    int m_audioStream = -1;
    AVCodecContext* m_audioCodecContext = nullptr;
    SwrContext* m_swrContext = nullptr;
    AVFrame* m_audioFrame = nullptr;
    AVIOContext* m_ioContext = nullptr;
    unsigned char* m_ioBuffer = nullptr;

    QAudioOutput* m_audioOutput = nullptr;
    QIODevice* m_audioDevice = nullptr;
    QAudioFormat m_audioFormat;

    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;
    bool m_stopped;
    bool m_paused = true;
    bool m_seeking = false;
    int64_t m_seekPosition = 0;
    double m_currentPosition = 0;
    bool m_reachedEndOfStream = false;
    bool m_initialized = false;
};

#endif // AUDIO_DECODER_H