#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4267 4996)
#endif

#include <QImage>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QIODevice>
#include <QDebug>
#include <QElapsedTimer>
#include <QBuffer>

#include "../../audio/decoder/audio_decoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif

class VideoDecoder : public QThread {
    Q_OBJECT

public:
    VideoDecoder(const QByteArray& videoData, QObject* parent = nullptr);

    ~VideoDecoder() override;

    bool initialize();

    void extractFirstFrame();

    void releaseResources() {
        cleanupFFmpegResources();
    }

    void reset();

    void stop();

    void pause();

    void seek(double position);

    void setVolume(float volume) {
        if (m_audioDecoder) {
            m_audioDecoder->setVolume(volume);
        }
    }

    float getVolume() const {
        return m_audioDecoder ? m_audioDecoder->getVolume() : 0.0f;
    }

    bool isPaused() const {
        QMutexLocker locker(&m_mutex);
        return m_paused;
    }

    bool hasAudio() const {
        return m_audioDecoder != nullptr;
    }

signals:
    void frameReady(const QImage& frame);
    void videoInfo(int width, int height, double fps, double duration, bool hasAudio);
    void positionChanged(double position);
    void playbackFinished();
    void error(const QString& message);

protected:
    void run() override;

private slots:
    void handleAudioError(const QString& message) {
        emit error("Błąd strumienia audio: " + message);
    }

    void handleAudioFinished();

    void updateAudioPosition(double position);

private:
    void cleanupFFmpegResources();

    static int readPacket(void* opaque, uint8_t* buf, int buf_size);

    static int64_t seekPacket(void* opaque, int64_t offset, int whence);

    QByteArray m_videoData;
    QBuffer m_buffer;
    AVIOContext* m_ioContext = nullptr;
    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    SwsContext* m_swsContext = nullptr;
    AVFrame* m_frame = nullptr;
    int m_videoStream = -1;
    double m_frameRate = 0.0;
    double m_duration = 0.0;
    double m_currentPosition = 0.0;

    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;
    bool m_stopped = false;
    bool m_paused = false;
    bool m_seeking = false;
    int64_t m_seekPosition = -1;

    QElapsedTimer m_frameTimer;
    bool m_firstFrame = true;
    int64_t m_lastFramePts = AV_NOPTS_VALUE;

    AudioDecoder* m_audioDecoder = nullptr;
    int m_audioStreamIndex = -1;
    double m_currentAudioPosition = 0.0;
    bool m_audioInitialized = false;
    bool m_audioFinished = false;
    bool m_videoFinished = false;
};

#endif // VIDEO_DECODER_H