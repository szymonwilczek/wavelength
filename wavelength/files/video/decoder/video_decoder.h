//
// Created by szymo on 16.03.2025.
//

#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#ifdef _MSC_VER
#pragma comment(lib, "swresample.lib")
#endif

#include <QImage>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QBuffer>
#include <QIODevice>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

class VideoDecoder : public QThread {
    Q_OBJECT

public:
    VideoDecoder(const QByteArray& videoData, QObject* parent = nullptr)
        : QThread(parent), m_videoData(videoData), m_stopped(false) {
        // Zainicjuj domyślne wartości
        m_formatContext = nullptr;
        m_codecContext = nullptr;
        m_swsContext = nullptr;
        m_frame = nullptr;
        m_frameRGB = nullptr;
        m_videoStream = -1;
        m_buffer = nullptr;
        m_bufferSize = 0;

        // Zainicjuj bufor z danymi wideo
        m_ioBuffer = (unsigned char*)av_malloc(m_videoData.size() + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!m_ioBuffer) {
            emit error("Nie można zaalokować pamięci dla danych wideo");
            return;
        }
        memcpy(m_ioBuffer, m_videoData.constData(), m_videoData.size());

        m_ioContext = avio_alloc_context(
            m_ioBuffer, m_videoData.size(), 0, this,
            &VideoDecoder::readPacket, nullptr, &VideoDecoder::seekPacket
        );
        if (!m_ioContext) {
            av_free(m_ioBuffer);
            m_ioBuffer = nullptr;
            emit error("Nie można utworzyć kontekstu I/O");
            return;
        }
    }

    ~VideoDecoder() {
        stop();
        wait();

        if (m_buffer)
            av_free(m_buffer);

        if (m_frameRGB)
            av_frame_free(&m_frameRGB);

        if (m_frame)
            av_frame_free(&m_frame);

        if (m_codecContext)
            avcodec_free_context(&m_codecContext);

        // Zwolnij zasoby audio
        if (m_audioCodecContext)
            avcodec_free_context(&m_audioCodecContext);

        if (m_swrContext)
            swr_free(&m_swrContext);

        if (m_audioOutput) {
            m_audioOutput->stop();
            delete m_audioOutput;
        }

        if (m_formatContext) {
            if (m_formatContext->pb == m_ioContext)
                m_formatContext->pb = nullptr;
            avformat_close_input(&m_formatContext);
        }

        if (m_ioContext) {
            av_free(m_ioContext->buffer);
            avio_context_free(&m_ioContext);
        }
    }

    void setVolume(float volume) {
        if (m_audioOutput) {
            m_audioOutput->setVolume(volume);
        }
    }

    float getVolume() const {
        return m_audioOutput ? m_audioOutput->volume() : 0.0f;
    }

    bool hasAudio() const {
        return m_hasAudio;
    }

    bool initialize() {
        // Rejestruj wszystkie kodeki i formaty
        static bool registered = false;
        if (!registered) {
            registered = true;
        }

        // Utwórz kontekst formatu
        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) {
            emit error("Nie można utworzyć kontekstu formatu");
            return false;
        }

        m_formatContext->pb = m_ioContext;

        // Otwórz strumień wideo
        if (avformat_open_input(&m_formatContext, "", nullptr, nullptr) < 0) {
            emit error("Nie można otworzyć strumienia wideo");
            return false;
        }

        // Znajdź informacje o strumieniu
        if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
            emit error("Nie można znaleźć informacji o strumieniu");
            return false;
        }

        // Znajdź pierwszy strumień wideo
        m_videoStream = -1;
        m_audioStream = -1;
        for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
            if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                m_videoStream = i;
            } else if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                m_audioStream = i;
            }
        }

        if (m_videoStream == -1) {
            emit error("Nie znaleziono strumienia wideo");
            return false;
        }

        // Znajdź dekoder
        const AVCodec* codec = avcodec_find_decoder(m_formatContext->streams[m_videoStream]->codecpar->codec_id);
        if (!codec) {
            emit error("Nie znaleziono dekodera dla tego formatu wideo");
            return false;
        }

        // Utwórz kontekst kodeka
        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) {
            emit error("Nie można utworzyć kontekstu kodeka");
            return false;
        }

        // Skopiuj parametry kodeka
        if (avcodec_parameters_to_context(m_codecContext, m_formatContext->streams[m_videoStream]->codecpar) < 0) {
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
            emit error("Nie można zaalokować ramek wideo");
            return false;
        }

        // Określ rozmiar bufora dla skonwertowanych ramek
        m_bufferSize = av_image_get_buffer_size(
            AV_PIX_FMT_RGB24,
            m_codecContext->width,
            m_codecContext->height,
            1
        );

        m_buffer = (uint8_t*)av_malloc(m_bufferSize);
        if (!m_buffer) {
            emit error("Nie można zaalokować bufora wideo");
            return false;
        }

        // Ustaw dane wyjściowe w frameRGB
        av_image_fill_arrays(
            m_frameRGB->data, m_frameRGB->linesize, m_buffer,
            AV_PIX_FMT_RGB24, m_codecContext->width, m_codecContext->height, 1
        );

        // Utwórz kontekst konwersji
        m_swsContext = sws_getContext(
            m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
            m_codecContext->width, m_codecContext->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (!m_swsContext) {
            emit error("Nie można utworzyć kontekstu konwersji");
            return false;
        }

        // Wyemituj informacje o wideo
        emit videoInfo(
            m_codecContext->width,
            m_codecContext->height,
            m_formatContext->duration / AV_TIME_BASE
        );

        if (m_audioStream != -1) {
            if (!initializeAudio()) {
                qDebug() << "Nie można zainicjalizować audio, odtwarzam tylko wideo";
                m_hasAudio = false;
            } else {
                m_hasAudio = true;
            }
        }

        return true;
    }

    bool initializeAudio() {
        // Znajdź dekoder audio
        const AVCodec* audioCodec = avcodec_find_decoder(
            m_formatContext->streams[m_audioStream]->codecpar->codec_id
        );
        if (!audioCodec) {
            qDebug() << "Nie znaleziono dekodera audio";
            return false;
        }

        // Utwórz kontekst kodeka audio
        m_audioCodecContext = avcodec_alloc_context3(audioCodec);
        if (!m_audioCodecContext) {
            qDebug() << "Nie można utworzyć kontekstu kodeka audio";
            return false;
        }

        // Skopiuj parametry kodeka audio
        if (avcodec_parameters_to_context(
                m_audioCodecContext,
                m_formatContext->streams[m_audioStream]->codecpar
            ) < 0) {
            qDebug() << "Nie można skopiować parametrów kodeka audio";
            return false;
        }

        // Otwórz kodek audio
        if (avcodec_open2(m_audioCodecContext, audioCodec, nullptr) < 0) {
            qDebug() << "Nie można otworzyć kodeka audio";
            return false;
        }

        // Utwórz kontekst resamplingu
        m_swrContext = swr_alloc();
        if (!m_swrContext) {
            qDebug() << "Nie można utworzyć kontekstu resamplingu audio";
            return false;
        }

        // Inicjalizacja layoutu kanałów wyjściowych (stereo)
        AVChannelLayout outLayout;
        av_channel_layout_default(&outLayout, 2); // 2 kanały (stereo)

        // Konfiguracja resamplingu do formatu PCM S16LE stereo 44.1kHz
        av_opt_set_chlayout(m_swrContext, "in_chlayout", &m_audioCodecContext->ch_layout, 0);
        av_opt_set_chlayout(m_swrContext, "out_chlayout", &outLayout, 0);
        av_opt_set_int(m_swrContext, "in_sample_rate", m_audioCodecContext->sample_rate, 0);
        av_opt_set_int(m_swrContext, "out_sample_rate", 44100, 0);
        av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt", m_audioCodecContext->sample_fmt, 0);
        av_opt_set_sample_fmt(m_swrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

        if (swr_init(m_swrContext) < 0) {
            qDebug() << "Nie można zainicjalizować kontekstu resamplingu audio";
            return false;
        }

        // Zwolnij pamięć layoutu kanałów wyjściowych
        av_channel_layout_uninit(&outLayout);

        // Konfiguracja formatu audio dla Qt
        m_audioFormat.setSampleRate(44100);
        m_audioFormat.setChannelCount(2);
        m_audioFormat.setSampleType(QAudioFormat::SignedInt); // Poprawna wartość enumeracji
        m_audioFormat.setSampleSize(16); // Dodajemy rozmiar próbki (16 bitów)
        m_audioFormat.setCodec("audio/pcm");

        // Inicjalizacja wyjścia audio
        m_audioOutput = new QAudioOutput(m_audioFormat, nullptr);
        m_audioOutput->setVolume(1.0);
        m_audioOutputBuffer.setBuffer(&m_audioBuffer);
        m_audioOutputBuffer.open(QIODevice::ReadWrite);
        m_audioDevice = m_audioOutput->start();

        return true;
    }

    void stop() {
        m_mutex.lock();
        m_stopped = true;
        m_mutex.unlock();

        m_waitCondition.wakeOne();
    }

    void pause() {
        m_mutex.lock();
        m_paused = !m_paused;
        m_mutex.unlock();

        if (!m_paused)
            m_waitCondition.wakeOne();
    }

    bool isPaused() const {
        return m_paused;
    }

    void seek(double position) {
        int64_t timestamp = position * AV_TIME_BASE;
        timestamp = av_rescale_q(timestamp, AV_TIME_BASE_Q, m_formatContext->streams[m_videoStream]->time_base);

        m_mutex.lock();
        m_seekPosition = timestamp;
        m_seeking = true;
        m_mutex.unlock();

        if (m_paused)
            m_waitCondition.wakeOne();
    }

signals:
    void frameReady(const QImage& frame);
    void error(const QString& message);
    void videoInfo(int width, int height, double duration);
    void playbackFinished();
    void positionChanged(double position);

protected:
    void run() override {
        if (!initialize()) {
            return;
        }

        AVPacket packet;
        bool frameFinished;
        AVFrame* audioFrame = av_frame_alloc();

        while (!m_stopped) {
            m_mutex.lock();

            if (m_paused) {
                m_waitCondition.wait(&m_mutex);
                m_mutex.unlock();
                continue;
            }

            if (m_seeking) {
                av_seek_frame(m_formatContext, m_videoStream, m_seekPosition, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(m_codecContext);
                m_seeking = false;

                // Po przeszukiwaniu wyczyść stan i zaktualizuj pozycję
                m_currentPosition = m_seekPosition * av_q2d(m_formatContext->streams[m_videoStream]->time_base);
                emit positionChanged(m_currentPosition);
            }

            m_mutex.unlock();

            if (av_read_frame(m_formatContext, &packet) < 0) {
                // Koniec strumienia
                emit playbackFinished();
                break;
            }

            if (packet.stream_index == m_videoStream) {
                // Dekoduj ramkę wideo
                avcodec_send_packet(m_codecContext, &packet);
                frameFinished = (avcodec_receive_frame(m_codecContext, m_frame) == 0);

                if (frameFinished) {
                    // Aktualizuj rzeczywistą pozycję odtwarzania na podstawie timestampa ramki
                    if (m_frame->pts != AV_NOPTS_VALUE) {
                        m_currentPosition = m_frame->pts * av_q2d(m_formatContext->streams[m_videoStream]->time_base);
                        emit positionChanged(m_currentPosition);
                    }

                    // Konwertuj ramkę do RGB
                    sws_scale(
                        m_swsContext,
                        (const uint8_t* const*)m_frame->data, m_frame->linesize, 0, m_codecContext->height,
                        m_frameRGB->data, m_frameRGB->linesize
                    );

                    // Konwertuj do QImage
                    QImage frame(
                        m_frameRGB->data[0],
                        m_codecContext->width,
                        m_codecContext->height,
                        m_frameRGB->linesize[0],
                        QImage::Format_RGB888
                    );

                    // Wyemituj ramkę
                    emit frameReady(frame.copy());

                    // Zmniejsz sleep, aby uzyskać ~60fps
                    QThread::msleep(16);  // ~60fps
                }
            } else if (m_hasAudio && packet.stream_index == m_audioStream) {
                // Dekoduj ramkę audio
                avcodec_send_packet(m_audioCodecContext, &packet);
                if (avcodec_receive_frame(m_audioCodecContext, audioFrame) == 0) {
                    // Konwertuj audio do PCM i odtwarzaj
                    decodeAudioFrame(audioFrame);
                }
            }

            av_packet_unref(&packet);
        }
        av_frame_free(&audioFrame);
    }

    void decodeAudioFrame(AVFrame* audioFrame) {
        if (!m_hasAudio || !m_audioDevice)
            return;

        // Oblicz rozmiar bufora wyjściowego (2 kanały stereo, 16-bit)
        int outSamples = audioFrame->nb_samples;
        int outBufferSize = outSamples * 2 * 2; // samples * channels * bytes_per_sample
        uint8_t* outBuffer = (uint8_t*)av_malloc(outBufferSize);

        int convertedSamples = swr_convert(
            m_swrContext,
            &outBuffer, outSamples,
            (const uint8_t**)audioFrame->extended_data, audioFrame->nb_samples
        );

        if (convertedSamples > 0) {
            int actualSize = convertedSamples * 2 * 2; // converted_samples * channels * bytes_per_sample
            QByteArray buffer((char*)outBuffer, actualSize);

            m_mutex.lock();
            m_audioOutputBuffer.write(buffer);

            if (m_audioBuffer.size() > 8192 && !m_paused) {
                m_audioOutputBuffer.seek(0);
                m_audioDevice->write(m_audioOutputBuffer.readAll());
                m_audioBuffer.clear();
                m_audioOutputBuffer.seek(0);
            }
            m_mutex.unlock();
        }

        av_freep(&outBuffer);
    }

private:
    // Funkcje callback dla custom I/O
    static int readPacket(void* opaque, uint8_t* buf, int buf_size) {
        VideoDecoder* decoder = static_cast<VideoDecoder*>(opaque);
        int size = qMin(buf_size, decoder->m_readPosition >= decoder->m_videoData.size() ?
            0 : decoder->m_videoData.size() - decoder->m_readPosition);

        if (size <= 0)
            return AVERROR_EOF;

        memcpy(buf, decoder->m_videoData.constData() + decoder->m_readPosition, size);
        decoder->m_readPosition += size;
        return size;
    }

    static int64_t seekPacket(void* opaque, int64_t offset, int whence) {
        VideoDecoder* decoder = static_cast<VideoDecoder*>(opaque);

        switch (whence) {
            case SEEK_SET:
                decoder->m_readPosition = offset;
                break;
            case SEEK_CUR:
                decoder->m_readPosition += offset;
                break;
            case SEEK_END:
                decoder->m_readPosition = decoder->m_videoData.size() + offset;
                break;
            case AVSEEK_SIZE:
                return decoder->m_videoData.size();
            default:
                return -1;
        }

        return decoder->m_readPosition;
    }

    QByteArray m_videoData;
    int m_readPosition = 0;

    AVFormatContext* m_formatContext;
    AVCodecContext* m_codecContext;
    SwsContext* m_swsContext;
    AVFrame* m_frame;
    AVFrame* m_frameRGB;
    AVIOContext* m_ioContext;
    unsigned char* m_ioBuffer;
    int m_videoStream;
    uint8_t* m_buffer;
    int m_bufferSize;

    int m_audioStream = -1;
    AVCodecContext* m_audioCodecContext = nullptr;
    SwrContext* m_swrContext = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QIODevice* m_audioDevice = nullptr;
    QByteArray m_audioBuffer;
    QBuffer m_audioOutputBuffer;
    bool m_hasAudio = false;
    QAudioFormat m_audioFormat;


    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    bool m_stopped;
    bool m_paused = true;
    bool m_seeking = false;
    int64_t m_seekPosition = 0;
    double m_currentPosition = 0;
};



#endif //VIDEO_DECODER_H
