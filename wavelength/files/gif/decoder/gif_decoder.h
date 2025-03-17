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

class GifDecoder : public QThread {
    Q_OBJECT

public:
    GifDecoder(const QByteArray& gifData, QObject* parent = nullptr)
        : QThread(parent), m_gifData(gifData), m_stopped(false) {
        // Zainicjuj domyślne wartości
        m_formatContext = nullptr;
        m_codecContext = nullptr;
        m_swsContext = nullptr;
        m_frame = nullptr;
        m_frameRGB = nullptr;
        m_gifStream = -1;
        m_buffer = nullptr;
        m_bufferSize = 0;
        m_paused = true;
        m_seeking = false;
        m_currentPosition = 0;
        m_reachedEndOfStream = false;
        m_initialized = false;
        m_frameDelay = 100; // Domyślne opóźnienie w ms

        // Zainicjuj bufor z danymi GIF
        m_ioBuffer = (unsigned char*)av_malloc(m_gifData.size() + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!m_ioBuffer) {
            emit error("Nie można zaalokować pamięci dla danych GIF");
            return;
        }
        memcpy(m_ioBuffer, m_gifData.constData(), m_gifData.size());

        m_ioContext = avio_alloc_context(
            m_ioBuffer, m_gifData.size(), 0, this,
            &GifDecoder::readPacket, nullptr, &GifDecoder::seekPacket
        );
        if (!m_ioContext) {
            av_free(m_ioBuffer);
            m_ioBuffer = nullptr;
            emit error("Nie można utworzyć kontekstu I/O");
            return;
        }
    }

    ~GifDecoder() {
        // Zatrzymaj wątek
        stop();
        wait(500); // Daj mu trochę czasu na zakończenie

        // Zwolnij zasoby FFmpeg i inne zasoby
        releaseResources();

        // Zwolnij zasoby IO, które są trzymane oddzielnie
        if (m_ioContext) {
            if (m_ioContext->buffer) {
                av_free(m_ioContext->buffer);
                m_ioContext->buffer = nullptr;
            }
            avio_context_free(&m_ioContext);
            m_ioContext = nullptr;
        } else if (m_ioBuffer) {
            av_free(m_ioBuffer);
            m_ioBuffer = nullptr;
        }
    }

    void releaseResources() {
        QMutexLocker locker(&m_mutex);
        
        if (m_buffer) {
            av_free(m_buffer);
            m_buffer = nullptr;
        }

        if (m_frameRGB) {
            av_frame_free(&m_frameRGB);
            m_frameRGB = nullptr;
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

        if (m_swsContext) {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }

        if (m_formatContext) {
            if (m_formatContext->pb == m_ioContext)
                m_formatContext->pb = nullptr;
            avformat_close_input(&m_formatContext);
            m_formatContext = nullptr;
        }

        m_initialized = false;
    }

    bool reinitialize() {
        QMutexLocker locker(&m_mutex);

        // Zatrzymaj wątek jeśli działa
        bool wasRunning = isRunning();
        if (wasRunning) {
            m_stopped = true;
            m_waitCondition.wakeAll();
            locker.unlock();
            wait(500); // Czekaj max 500ms
            locker.relock();
        }

        // Zwolnij wszystkie zasoby
        releaseResources();

        // Zresetuj stany
        m_stopped = false;
        m_paused = true;
        m_seeking = false;
        m_currentPosition = 0;
        m_readPosition = 0;
        m_reachedEndOfStream = false;

        // Przygotowanie buforów I/O jeśli zostały zwolnione
        if (!m_ioBuffer) {
            m_ioBuffer = (unsigned char*)av_malloc(m_gifData.size() + AV_INPUT_BUFFER_PADDING_SIZE);
            if (!m_ioBuffer) {
                emit error("Nie można zaalokować pamięci dla danych GIF");
                return false;
            }
            memcpy(m_ioBuffer, m_gifData.constData(), m_gifData.size());
        }

        if (!m_ioContext) {
            m_ioContext = avio_alloc_context(
                m_ioBuffer, m_gifData.size(), 0, this,
                &GifDecoder::readPacket, nullptr, &GifDecoder::seekPacket
            );
            if (!m_ioContext) {
                av_free(m_ioBuffer);
                m_ioBuffer = nullptr;
                emit error("Nie można utworzyć kontekstu I/O");
                return false;
            }
        }

        return true;
    }

    bool initialize() {
        QMutexLocker locker(&m_mutex);

        if (m_initialized) return true;  // Nie inicjalizuj ponownie, jeśli już zainicjalizowano

        // Utwórz kontekst formatu
        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) {
            emit error("Nie można utworzyć kontekstu formatu");
            return false;
        }

        m_formatContext->pb = m_ioContext;

        // Otwórz strumień GIF
        if (avformat_open_input(&m_formatContext, "", nullptr, nullptr) < 0) {
            emit error("Nie można otworzyć strumienia GIF");
            return false;
        }

        // Znajdź informacje o strumieniu
        if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
            emit error("Nie można znaleźć informacji o strumieniu");
            return false;
        }

        // Znajdź strumień wideo (GIF)
        m_gifStream = -1;
        for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
            if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                m_gifStream = i;
                break;
            }
        }

        if (m_gifStream == -1) {
            emit error("Nie znaleziono strumienia GIF");
            return false;
        }

        // Ustal częstotliwość klatek
        AVStream* stream = m_formatContext->streams[m_gifStream];
        if (stream->avg_frame_rate.den && stream->avg_frame_rate.num) {
            m_frameRate = av_q2d(stream->avg_frame_rate);
            m_frameDelay = 1000.0 / m_frameRate; // W milisekundach
        } else if (stream->r_frame_rate.den && stream->r_frame_rate.num) {
            m_frameRate = av_q2d(stream->r_frame_rate);
            m_frameDelay = 1000.0 / m_frameRate; // W milisekundach
        } else {
            m_frameRate = 10; // Domyślna wartość, jeśli nie można wykryć
            m_frameDelay = 100; // 100 ms = 10 FPS
        }

        qDebug() << "Wykryta częstotliwość klatek GIF:" << m_frameRate << "FPS, opóźnienie:" << m_frameDelay << "ms";

        // Znajdź dekoder
        const AVCodec* codec = avcodec_find_decoder(m_formatContext->streams[m_gifStream]->codecpar->codec_id);
        if (!codec) {
            emit error("Nie znaleziono dekodera dla formatu GIF");
            return false;
        }

        // Utwórz kontekst kodeka
        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) {
            emit error("Nie można utworzyć kontekstu kodeka");
            return false;
        }

        // Skopiuj parametry kodeka
        if (avcodec_parameters_to_context(m_codecContext, m_formatContext->streams[m_gifStream]->codecpar) < 0) {
            emit error("Nie można skopiować parametrów kodeka");
            return false;
        }

        // Otwórz kodek
        if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
            emit error("Nie można otworzyć kodeka");
            return false;
        }

        // Alokuj ramki
        m_frame = av_frame_alloc();
        m_frameRGB = av_frame_alloc();

        if (!m_frame || !m_frameRGB) {
            emit error("Nie można zaalokować ramek GIF");
            return false;
        }

        // Określ rozmiar bufora dla skonwertowanych ramek
        m_bufferSize = av_image_get_buffer_size(
            AV_PIX_FMT_RGBA,
            m_codecContext->width,
            m_codecContext->height,
            1
        );

        m_buffer = (uint8_t*)av_malloc(m_bufferSize);
        if (!m_buffer) {
            emit error("Nie można zaalokować bufora GIF");
            return false;
        }

        // Ustaw dane wyjściowe w frameRGB
        av_image_fill_arrays(
            m_frameRGB->data, m_frameRGB->linesize, m_buffer,
            AV_PIX_FMT_RGBA, m_codecContext->width, m_codecContext->height, 1
        );

        // Utwórz kontekst konwersji
        // Używamy RGBA, aby obsłużyć przezroczystość GIF-ów
        m_swsContext = sws_getContext(
            m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
            m_codecContext->width, m_codecContext->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (!m_swsContext) {
            emit error("Nie można utworzyć kontekstu konwersji");
            return false;
        }

        // Wyemituj informacje o GIF-ie
        emit gifInfo(
            m_codecContext->width,
            m_codecContext->height,
            m_formatContext->duration / AV_TIME_BASE,
            m_frameRate,
            m_formatContext->nb_streams  // Liczba strumieni (powinno być tylko 1 dla GIF)
        );

        m_initialized = true;
        return true;
    }

    void extractFirstFrame() {
        if (!m_formatContext || m_gifStream < 0)
            return;

        // Tymczasowo zapisz stan
        bool wasPaused = m_paused;

        // Ustaw na początek strumienia
        av_seek_frame(m_formatContext, m_gifStream, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(m_codecContext);

        // Odczytaj pakiet i dekoduj ramkę
        AVPacket packet;
        while (av_read_frame(m_formatContext, &packet) >= 0) {
            if (packet.stream_index == m_gifStream) {
                avcodec_send_packet(m_codecContext, &packet);
                if (avcodec_receive_frame(m_codecContext, m_frame) == 0) {
                    // Konwertuj ramkę do RGB
                    sws_scale(
                        m_swsContext,
                        (const uint8_t* const*)m_frame->data, m_frame->linesize, 0, m_codecContext->height,
                        m_frameRGB->data, m_frameRGB->linesize
                    );

                    // Utwórz QImage z obsługą przezroczystości (Format_RGBA8888)
                    QImage frame(
                        m_frameRGB->data[0],
                        m_codecContext->width,
                        m_codecContext->height,
                        m_frameRGB->linesize[0],
                        QImage::Format_RGBA8888
                    );

                    emit frameReady(frame);
                    av_packet_unref(&packet);
                    break;
                }
            }
            av_packet_unref(&packet);
        }

        // Przywróć stan pauzy
        m_paused = wasPaused;
    }

    void stop() {
        QMutexLocker locker(&m_mutex);
        m_stopped = true;
        locker.unlock();
        m_waitCondition.wakeOne();
    }

    void pause() {
        QMutexLocker locker(&m_mutex);
        m_paused = !m_paused;
        locker.unlock();

        if (!m_paused)
            m_waitCondition.wakeOne();
    }

    bool isPaused() const {
        const QMutex *locker(&m_mutex);
        return m_paused;
    }

    void seek(double position) {
        QMutexLocker locker(&m_mutex);

        if (!m_formatContext || m_gifStream < 0) return;

        int64_t timestamp = position * AV_TIME_BASE;

        // Bezpieczne ograniczenie pozycji
        if (timestamp < 0) timestamp = 0;
        if (timestamp > m_formatContext->duration)
            timestamp = m_formatContext->duration > 0 ? m_formatContext->duration - 1 : 0;

        timestamp = av_rescale_q(timestamp, AV_TIME_BASE_Q,
                                m_formatContext->streams[m_gifStream]->time_base);

        m_seekPosition = timestamp;
        m_seeking = true;
        locker.unlock();

        if (m_paused)
            m_waitCondition.wakeOne();
    }

    void reset() {
        QMutexLocker locker(&m_mutex);

        // Resetuj pozycję strumienia na początek
        if (m_formatContext && m_gifStream >= 0) {
            av_seek_frame(m_formatContext, m_gifStream, 0, AVSEEK_FLAG_BACKWARD);
            if (m_codecContext) {
                avcodec_flush_buffers(m_codecContext);
            }
            m_currentPosition = 0;
            emit positionChanged(0);
        }

        // Resetuj flagi stanu
        m_seeking = false;
        m_seekPosition = 0;
        m_reachedEndOfStream = false;
    }

    bool isRunning() const {
        return QThread::isRunning();
    }

    double getDuration() const {
        if (m_formatContext && m_formatContext->duration != AV_NOPTS_VALUE) {
            return m_formatContext->duration / (double)AV_TIME_BASE;
        }
        return 0.0;
    }

signals:
    void frameReady(const QImage& frame);
    void error(const QString& message);
    void gifInfo(int width, int height, double duration, double frameRate, int numStreams);
    void playbackFinished();
    void positionChanged(double position);

protected:
    void run() override {
        if (!initialize()) {
            return;
        }

        AVPacket packet;
        bool frameFinished;

        m_frameTimer.start();
        bool isFirstFrame = true;

        while (!m_stopped) {
            QMutexLocker locker(&m_mutex);

            if (m_paused) {
                m_waitCondition.wait(&m_mutex);
                if (!m_paused) {
                    m_frameTimer.restart();
                    isFirstFrame = true;
                }
                locker.unlock();
                continue;
            }

            if (m_seeking) {
                if (m_formatContext && m_gifStream >= 0) {
                    av_seek_frame(m_formatContext, m_gifStream, m_seekPosition, AVSEEK_FLAG_BACKWARD);
                    avcodec_flush_buffers(m_codecContext);
                    m_seeking = false;
                    m_frameTimer.restart();
                    isFirstFrame = true;

                    // Aktualizacja pozycji po przeszukiwaniu
                    m_currentPosition = m_seekPosition * av_q2d(m_formatContext->streams[m_gifStream]->time_base);
                    emit positionChanged(m_currentPosition);
                }
            }
            locker.unlock();

            // Czytaj kolejny pakiet
            int readResult = av_read_frame(m_formatContext, &packet);
            if (readResult < 0) {
                // W przypadku GIF-a, zazwyczaj chcemy zapętlić odtwarzanie
                // zamiast zakończyć. GIF-y są zwykle przeznaczone do ciągłego odtwarzania.
                QMutexLocker locker2(&m_mutex);
                av_seek_frame(m_formatContext, m_gifStream, 0, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(m_codecContext);
                m_currentPosition = 0;
                emit positionChanged(0);
                locker2.unlock();
                continue;
            }

            if (packet.stream_index == m_gifStream) {
                // Dekoduj ramkę GIF
                avcodec_send_packet(m_codecContext, &packet);
                frameFinished = (avcodec_receive_frame(m_codecContext, m_frame) == 0);

                if (frameFinished) {
                    // Aktualizacja pozycji odtwarzania
                    if (m_frame->pts != AV_NOPTS_VALUE) {
                        QMutexLocker posLocker(&m_mutex);
                        m_currentPosition = m_frame->pts *
                                          av_q2d(m_formatContext->streams[m_gifStream]->time_base);
                        posLocker.unlock();
                        emit positionChanged(m_currentPosition);
                    }

                    // Konwertuj ramkę do RGBA (z obsługą przezroczystości)
                    sws_scale(
                        m_swsContext,
                        (const uint8_t* const*)m_frame->data, m_frame->linesize, 0, m_codecContext->height,
                        m_frameRGB->data, m_frameRGB->linesize
                    );

                    // Twórz QImage z obsługą przezroczystości
                    QImage frame(
                        m_frameRGB->data[0],
                        m_codecContext->width,
                        m_codecContext->height,
                        m_frameRGB->linesize[0],
                        QImage::Format_RGBA8888
                    );

                    // Wykorzystaj mechanizm copy-on-write w QImage
                    emit frameReady(frame);

                    // Odczekaj odpowiedni czas na następną ramkę, zgodnie z opóźnieniem GIF lub obliczonym opóźnieniem
                    if (!isFirstFrame) {
                        int elapsed = m_frameTimer.elapsed();
                        int delayTime = qMax(0, static_cast<int>(m_frameDelay) - elapsed);
                        if (delayTime > 0) {
                            QThread::msleep(delayTime);
                        }
                    }

                    isFirstFrame = false;
                    m_frameTimer.restart();
                }
            }

            av_packet_unref(&packet);
        }
    }

private:
    // Funkcje callback dla custom I/O
    static int readPacket(void* opaque, uint8_t* buf, int buf_size) {
        GifDecoder* decoder = static_cast<GifDecoder*>(opaque);
        int size = qMin(buf_size, decoder->m_readPosition >= decoder->m_gifData.size() ?
            0 : decoder->m_gifData.size() - decoder->m_readPosition);

        if (size <= 0)
            return AVERROR_EOF;

        memcpy(buf, decoder->m_gifData.constData() + decoder->m_readPosition, size);
        decoder->m_readPosition += size;
        return size;
    }

    static int64_t seekPacket(void* opaque, int64_t offset, int whence) {
        GifDecoder* decoder = static_cast<GifDecoder*>(opaque);

        switch (whence) {
            case SEEK_SET:
                decoder->m_readPosition = offset;
                break;
            case SEEK_CUR:
                decoder->m_readPosition += offset;
                break;
            case SEEK_END:
                decoder->m_readPosition = decoder->m_gifData.size() + offset;
                break;
            case AVSEEK_SIZE:
                return decoder->m_gifData.size();
            default:
                return -1;
        }

        return decoder->m_readPosition;
    }

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