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
    explicit GifDecoder(const QByteArray& gifData, QObject* parent = nullptr);

    ~GifDecoder() override;

    void releaseResources();

    bool reinitialize();

    bool initialize();

    void stop();

    void pause();

    bool isPaused() const {
        return m_paused;
    }

    void seek(double position);

    void reset();

    bool isDecoderRunning() const {
        return QThread::isRunning();
    }

    double getDuration() const {
        if (m_formatContext && m_formatContext->duration != AV_NOPTS_VALUE) {
            return m_formatContext->duration / static_cast<double>(AV_TIME_BASE);
        }
        return 0.0;
    }

    void resume();

signals:
    void frameReady(const QImage& frame);
    void firstFrameReady(const QImage& frame);
    void error(const QString& message);
    void gifInfo(int width, int height, double duration, double frameRate, int numStreams);
    void playbackFinished();
    void positionChanged(double position);

protected:
     void run() override;

private:
    // Funkcje callback dla custom I/O
    static int readPacket(void* opaque, uint8_t* buf, int buf_size);

    static int64_t seekPacket(void* opaque, int64_t offset, int whence);

    void extractAndEmitFirstFrameInternal();

    QByteArray m_gifData;
    int m_readPosition = 0;

    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    SwsContext* m_swsContext = nullptr;
    AVFrame* m_frame = nullptr;
    AVFrame* m_frameRGB = nullptr;
    AVIOContext* m_ioContext = nullptr;
    unsigned char* m_ioBuffer = nullptr;
    int m_gifStream = -1;
    uint8_t* m_buffer = nullptr;
    int m_bufferSize = 0;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    bool m_stopped = false;
    bool m_paused = true;
    bool m_seeking = false;
    int64_t m_seekPosition = 0;
    double m_currentPosition = 0;
    bool m_reachedEndOfStream = false;
    bool m_initialized = false;

    double m_frameRate = 0.0;
    double m_frameDelay = 100.0; // ms
    QElapsedTimer m_frameTimer;
};

#endif // GIF_DECODER_H