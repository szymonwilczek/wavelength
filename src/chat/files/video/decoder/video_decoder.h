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
    VideoDecoder(const QByteArray& videoData, QObject* parent = nullptr)
        : QThread(parent), m_videoData(videoData), m_formatContext(nullptr),
          m_codecContext(nullptr),
          m_swsContext(nullptr),
          m_frame(nullptr), m_videoStream(-1), m_frameRate(0.0), m_duration(0.0),
          m_currentPosition(0.0), m_stopped(false), m_paused(true), m_seeking(false),
          m_seekPosition(-1), m_firstFrame(true), m_lastFramePts(AV_NOPTS_VALUE),
          m_audioDecoder(nullptr), m_audioStreamIndex(-1), m_currentAudioPosition(0.0),
          m_audioInitialized(false), m_audioFinished(false), m_videoFinished(false)
    {
        m_buffer.setData(m_videoData);
        m_buffer.open(QIODevice::ReadOnly);
        m_ioContext = avio_alloc_context(
            (unsigned char*)av_malloc(8192), 8192, 0, &m_buffer,
            &VideoDecoder::readPacket, nullptr, &VideoDecoder::seekPacket
        );
    }

    ~VideoDecoder() override {
        stop();
        wait();
        cleanupFFmpegResources();
        if (m_ioContext) {
            av_freep(&m_ioContext->buffer);
            avio_context_free(&m_ioContext);
            m_ioContext = nullptr;
        }
    }

    bool initialize() {
        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) return false;
        m_formatContext->pb = m_ioContext;
        m_formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

        if (avformat_open_input(&m_formatContext, nullptr, nullptr, nullptr) != 0) {
            emit error("Nie można otworzyć strumienia wideo (avformat_open_input)");
            return false;
        }

        if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
            emit error("Nie można znaleźć informacji o strumieniu (avformat_find_stream_info)");
            return false;
        }

        m_videoStream = -1;
        m_audioStreamIndex = -1;
        for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
            if (m_videoStream == -1 && m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                m_videoStream = i;
            } else if (m_audioStreamIndex == -1 && m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                m_audioStreamIndex = i;
            }
        }

        if (m_videoStream == -1) {
            emit error("Nie znaleziono strumienia wideo");
            return false;
        }

        AVCodecParameters* codecParams = m_formatContext->streams[m_videoStream]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) {
            emit error("Nie znaleziono kodeka wideo");
            return false;
        }

        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) return false;
        if (avcodec_parameters_to_context(m_codecContext, codecParams) < 0) return false;
        if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
            emit error("Nie można otworzyć kodeka wideo");
            return false;
        }

        m_frame = av_frame_alloc();
        if (!m_frame) return false;

        m_swsContext = sws_getContext(
            m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
            m_codecContext->width, m_codecContext->height, AV_PIX_FMT_RGB24,
            SWS_LANCZOS, nullptr, nullptr, nullptr
        );
        if (!m_swsContext) {
            emit error("Nie można utworzyć kontekstu konwersji wideo (sws_getContext)");
            return false;
        }

        AVRational frameRateRational = av_guess_frame_rate(m_formatContext, m_formatContext->streams[m_videoStream], nullptr);
        m_frameRate = frameRateRational.num > 0 && frameRateRational.den > 0 ? av_q2d(frameRateRational) : 30.0;
        m_duration = m_formatContext->duration > 0 ? (double)m_formatContext->duration / AV_TIME_BASE : 0.0;

        if (m_audioStreamIndex != -1) {
            m_audioDecoder = new AudioDecoder(m_videoData, nullptr);
            connect(m_audioDecoder, &AudioDecoder::error, this, &VideoDecoder::handleAudioError);
            connect(m_audioDecoder, &AudioDecoder::playbackFinished, this, &VideoDecoder::handleAudioFinished);
            connect(m_audioDecoder, &AudioDecoder::positionChanged, this, &VideoDecoder::updateAudioPosition);
            connect(m_audioDecoder, &AudioDecoder::audioInfo, this, &VideoDecoder::handleAudioInfo);
            m_audioInitialized = false;
        } else {
             qDebug() << "Nie znaleziono strumienia audio w pliku.";
        }

        emit videoInfo(m_codecContext->width, m_codecContext->height, m_frameRate, m_duration, hasAudio());
        return true;
    }

    void extractFirstFrame() {
        // Ta metoda działa synchronicznie i nie używa głównej pętli run()
        // ani zewnętrznego AudioDecoder.

        // 1. Podstawowa inicjalizacja FFmpeg (podobna do initialize(), ale bez audio)
        AVFormatContext* tempFormatCtx = avformat_alloc_context();
        if (!tempFormatCtx) return;

        // Użyj kopii bufora, aby nie zakłócać głównego odtwarzania
        QBuffer tempBuffer;
        tempBuffer.setData(m_videoData);
        tempBuffer.open(QIODevice::ReadOnly);

        AVIOContext* tempIoCtx = avio_alloc_context(
            (unsigned char*)av_malloc(8192), 8192, 0, &tempBuffer,
            &VideoDecoder::readPacket, nullptr, &VideoDecoder::seekPacket
        );
        if (!tempIoCtx) {
            avformat_free_context(tempFormatCtx);
            return;
        }
        tempFormatCtx->pb = tempIoCtx;
        tempFormatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

        if (avformat_open_input(&tempFormatCtx, nullptr, nullptr, nullptr) != 0) {
            av_freep(&tempIoCtx->buffer);
            avio_context_free(&tempIoCtx);
            avformat_free_context(tempFormatCtx);
            return;
        }
        if (avformat_find_stream_info(tempFormatCtx, nullptr) < 0) {
            avformat_close_input(&tempFormatCtx); // Zamknij przed zwolnieniem IO
            av_freep(&tempIoCtx->buffer);
            avio_context_free(&tempIoCtx);
            return;
        }

        // 2. Znajdź strumień wideo
        int videoStreamIdx = -1;
        for (unsigned int i = 0; i < tempFormatCtx->nb_streams; i++) {
            if (tempFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIdx = i;
                break;
            }
        }
        if (videoStreamIdx == -1) {
            avformat_close_input(&tempFormatCtx);
            av_freep(&tempIoCtx->buffer);
            avio_context_free(&tempIoCtx);
            return;
        }

        // 3. Otwórz kodek wideo
        AVCodecParameters* codecParams = tempFormatCtx->streams[videoStreamIdx]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) { /* cleanup */ return; }
        AVCodecContext* tempCodecCtx = avcodec_alloc_context3(codec);
        if (!tempCodecCtx) { /* cleanup */ return; }
        if (avcodec_parameters_to_context(tempCodecCtx, codecParams) < 0) { /* cleanup */ return; }
        if (avcodec_open2(tempCodecCtx, codec, nullptr) < 0) { /* cleanup */ return; }

        // 4. Przygotuj ramkę i kontekst konwersji
        AVFrame* tempFrame = av_frame_alloc();
        if (!tempFrame) { /* cleanup */ return; }
        SwsContext* tempSwsCtx = sws_getContext(
            tempCodecCtx->width, tempCodecCtx->height, tempCodecCtx->pix_fmt,
            tempCodecCtx->width, tempCodecCtx->height, AV_PIX_FMT_RGB24,
            SWS_LANCZOS, nullptr, nullptr, nullptr
        );
        if (!tempSwsCtx) { /* cleanup */ return; }

        // 5. Dekoduj aż do pierwszej klatki wideo
        AVPacket packet;
        bool frameDecoded = false;
        while (av_read_frame(tempFormatCtx, &packet) >= 0 && !frameDecoded) {
            if (packet.stream_index == videoStreamIdx) {
                if (avcodec_send_packet(tempCodecCtx, &packet) == 0) {
                    if (avcodec_receive_frame(tempCodecCtx, tempFrame) == 0) {
                        // Mamy klatkę! Konwertuj i emituj.
                        QImage frameImage(tempCodecCtx->width, tempCodecCtx->height, QImage::Format_RGB888);
                        uint8_t* dstData[4] = { frameImage.bits(), nullptr, nullptr, nullptr };
                        int dstLinesize[4] = { frameImage.bytesPerLine(), 0, 0, 0 };
                        sws_scale(tempSwsCtx, (const uint8_t* const*)tempFrame->data, tempFrame->linesize, 0,
                                  tempCodecCtx->height, dstData, dstLinesize);

                        emit frameReady(frameImage); // Emituj sygnał
                        frameDecoded = true;
                    }
                }
            }
            av_packet_unref(&packet);
        }

        // 6. Cleanup
        sws_freeContext(tempSwsCtx);
        av_frame_free(&tempFrame);
        avcodec_free_context(&tempCodecCtx);
        avformat_close_input(&tempFormatCtx);
        av_freep(&tempIoCtx->buffer);
        avio_context_free(&tempIoCtx);
    }

    void releaseResources() {
        cleanupFFmpegResources();
    }

    void reset() {
        QMutexLocker locker(&m_mutex);
        m_paused = true; // Zatrzymaj na czas resetu
        m_videoFinished = false;
        m_audioFinished = !hasAudio(); // Resetuj flagę audio
        m_firstFrame = true;
        m_lastFramePts = AV_NOPTS_VALUE;
        m_currentAudioPosition = 0.0; // Resetuj pozycję audio

        // Przewiń strumień wideo do początku
        m_seekPosition = 0;
        m_seeking = true; // Ustaw flagę seek, pętla run() to obsłuży

        locker.unlock();

        // Przewiń zewnętrzny dekoder audio
        if (m_audioDecoder) {
            m_audioDecoder->seek(0.0);
            // Upewnij się, że audio jest spauzowane
            if (!m_audioDecoder->isPaused()) {
                m_audioDecoder->pause();
            }
        }

        // Obudź wątek, jeśli czekał
        m_waitCondition.wakeOne();
    }

    void stop() {
        QMutexLocker locker(&m_mutex);
        if (m_stopped) return;
        m_stopped = true;
        if (m_audioDecoder) {
            m_audioDecoder->stop();
        }
        locker.unlock();
        m_waitCondition.wakeOne();
    }

    void pause() {
        m_mutex.lock();
        m_paused = !m_paused;
        if (m_audioDecoder) {
            if (m_paused != m_audioDecoder->isPaused()) {
                 m_audioDecoder->pause();
            }
        }
        m_mutex.unlock();

        if (!m_paused)
            m_waitCondition.wakeOne();
    }

    void seek(double position) {
        if (m_audioDecoder) {
            m_audioDecoder->seek(position);
        }

        int64_t timestamp = position * AV_TIME_BASE;
        timestamp = av_rescale_q(timestamp, AV_TIME_BASE_Q, m_formatContext->streams[m_videoStream]->time_base);

        m_mutex.lock();
        m_seekPosition = timestamp;
        m_seeking = true;
        m_videoFinished = false;
        m_audioFinished = !hasAudio(); // Reset audio finished only if audio exists
        m_mutex.unlock();

        if (m_paused)
            m_waitCondition.wakeOne();
    }

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
    void run() override {
        if (!m_formatContext && !initialize()) {
            return;
        }

        if (m_audioDecoder && !m_audioInitialized) {
            if (!m_audioDecoder->initialize()) {
                qWarning() << "Nie udało się zainicjalizować zewnętrznego AudioDecoder.";
                delete m_audioDecoder;
                m_audioDecoder = nullptr;
            } else {
                m_audioDecoder->start();
                m_audioInitialized = true;
                if (!m_audioDecoder->isPaused()) {
                    m_audioDecoder->pause();
                }
            }
        }

        AVPacket packet;
        bool frameFinished;
        double frameDuration = m_frameRate > 0 ? 1000.0 / m_frameRate : 33.3;

        m_frameTimer.start();
        m_firstFrame = true;
        m_videoFinished = false;
        m_audioFinished = !hasAudio();

        while (!m_stopped) {
            m_mutex.lock();
            while (m_paused && !m_stopped && !m_seeking) {
                m_waitCondition.wait(&m_mutex);
            }

            if (m_seeking) {
                avcodec_flush_buffers(m_codecContext);
                av_seek_frame(m_formatContext, m_videoStream, m_seekPosition, AVSEEK_FLAG_BACKWARD);
                m_seeking = false;
                m_firstFrame = true;
                m_lastFramePts = AV_NOPTS_VALUE;
                m_frameTimer.restart();
            }
            m_mutex.unlock();

            if (m_stopped) break;

            if (av_read_frame(m_formatContext, &packet) < 0) {
                qDebug() << "Koniec strumienia wideo (av_read_frame < 0)";
                QMutexLocker lock(&m_mutex);
                m_videoFinished = true;
                if (m_audioFinished) {
                    lock.unlock();
                    emit playbackFinished();
                    lock.relock();
                }
                m_paused = true;
                lock.unlock();
                QThread::msleep(50);
                continue;
            }

            if (packet.stream_index == m_videoStream) {
                avcodec_send_packet(m_codecContext, &packet);
                frameFinished = (avcodec_receive_frame(m_codecContext, m_frame) == 0);

                if (frameFinished) {
                    double videoPts = -1.0;
                    if (m_frame->pts != AV_NOPTS_VALUE) {
                        videoPts = m_frame->pts * av_q2d(m_formatContext->streams[m_videoStream]->time_base);
                    }

                    if (m_audioDecoder && videoPts >= 0) {
                        QMutexLocker syncLocker(&m_mutex);
                        double audioPts = m_currentAudioPosition;
                        syncLocker.unlock();

                        double diff = videoPts - audioPts;
                        const double syncThreshold = 0.05;
                        const double maxSleep = 100.0;

                        if (diff > syncThreshold) {
                            int sleepTime = qRound((diff - syncThreshold / 2.0) * 1000.0);
                            QThread::msleep(qMin((int)maxSleep, qMax(0, sleepTime)));
                        } else if (diff < -syncThreshold * 2.0) {
                            av_packet_unref(&packet);
                            av_frame_unref(m_frame);
                            continue;
                        }
                    } else if (!m_audioDecoder) {
                         if (!m_firstFrame) {
                             int64_t currentPts = m_frame->pts;
                             double ptsDiff = (currentPts - m_lastFramePts) *
                                              av_q2d(m_formatContext->streams[m_videoStream]->time_base) * 1000.0;
                             if (ptsDiff <= 0 || ptsDiff > 1000.0) {
                                 ptsDiff = frameDuration;
                             }
                             int elapsedTime = m_frameTimer.elapsed();
                             int sleepTime = qMax(0, qRound(ptsDiff - elapsedTime));
                             if (sleepTime > 0) {
                                 QThread::msleep(sleepTime);
                             }
                         }
                         m_firstFrame = false;
                         m_lastFramePts = m_frame->pts;
                         m_frameTimer.restart();
                    }

                    QImage frameImage(m_codecContext->width, m_codecContext->height, QImage::Format_RGB888);
                    uint8_t* dstData[4] = { frameImage.bits(), nullptr, nullptr, nullptr };
                    int dstLinesize[4] = { frameImage.bytesPerLine(), 0, 0, 0 };
                    sws_scale(m_swsContext, (const uint8_t* const*)m_frame->data, m_frame->linesize, 0,
                              m_codecContext->height, dstData, dstLinesize);

                    emit frameReady(frameImage);
                    av_frame_unref(m_frame);
                }
            } else if (packet.stream_index == m_audioStreamIndex) {
                // Ignoruj pakiety audio
            }

            av_packet_unref(&packet);
        }
        qDebug() << "VideoDecoder::run() zakończył pętlę.";
    }

private slots:
    void handleAudioError(const QString& message) {
        qWarning() << "Błąd dekodera audio:" << message;
        emit error("Błąd strumienia audio: " + message);
    }

    void handleAudioFinished() {
        qDebug() << "AudioDecoder zakończył odtwarzanie.";
        QMutexLocker locker(&m_mutex);
        m_audioFinished = true;
        if (m_videoFinished) {
             locker.unlock();
             emit playbackFinished();
        }
    }

    void updateAudioPosition(double position) {
        QMutexLocker locker(&m_mutex);
        m_currentAudioPosition = position;
        locker.unlock();
        emit positionChanged(position);
    }

    void handleAudioInfo(int sampleRate, int channels, double duration) {
         qDebug() << "Audio Info:" << sampleRate << "Hz," << channels << "ch, Duration:" << duration;
    }

private:
    void cleanupFFmpegResources() {
        if (m_audioDecoder) {
            m_audioDecoder->stop();
            m_audioDecoder->wait(200);
            delete m_audioDecoder;
            m_audioDecoder = nullptr;
        }
        m_audioInitialized = false;
        m_audioFinished = false;
        m_videoFinished = false;

        if (m_swsContext) {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }
        if (m_frame) {
            av_frame_free(&m_frame);
            m_frame = nullptr;
        }
        if (m_codecContext) {
            avcodec_close(m_codecContext);
            avcodec_free_context(&m_codecContext);
            m_codecContext = nullptr;
        }
        if (m_formatContext) {
            avformat_close_input(&m_formatContext);
            m_formatContext = nullptr;
        }
        m_audioStreamIndex = -1;
        m_videoStream = -1;
    }

    static int readPacket(void* opaque, uint8_t* buf, int buf_size) {
        QBuffer* buffer = static_cast<QBuffer*>(opaque);
        qint64 bytesRead = buffer->read((char*)buf, buf_size);
        return (int)bytesRead;
    }

    static int64_t seekPacket(void* opaque, int64_t offset, int whence) {
        QBuffer* buffer = static_cast<QBuffer*>(opaque);
        if (whence == AVSEEK_SIZE) {
            return buffer->size();
        }
        if (buffer->seek(offset)) {
            return buffer->pos();
        }
        return -1;
    }

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