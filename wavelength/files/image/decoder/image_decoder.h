#ifndef IMAGE_DECODER_H
#define IMAGE_DECODER_H

#ifdef _MSC_VER
#pragma comment(lib, "swscale.lib")
#endif

#include <QImage>
#include <QMutex>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class ImageDecoder : public QObject {
    Q_OBJECT

public:
    ImageDecoder(const QByteArray& imageData, QObject* parent = nullptr)
        : QObject(parent), m_imageData(imageData) {
        // Zainicjuj domyślne wartości
        m_formatContext = nullptr;
        m_codecContext = nullptr;
        m_swsContext = nullptr;
        m_frame = nullptr;
        m_frameRGB = nullptr;
        m_imageStream = -1;
        m_buffer = nullptr;
        m_bufferSize = 0;
        m_initialized = false;

        // Zainicjuj bufor z danymi obrazu
        m_ioBuffer = (unsigned char*)av_malloc(m_imageData.size() + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!m_ioBuffer) {
            emit error("Nie można zaalokować pamięci dla danych obrazu");
            return;
        }
        memcpy(m_ioBuffer, m_imageData.constData(), m_imageData.size());

        m_ioContext = avio_alloc_context(
            m_ioBuffer, m_imageData.size(), 0, this,
            &ImageDecoder::readPacket, nullptr, &ImageDecoder::seekPacket
        );
        if (!m_ioContext) {
            av_free(m_ioBuffer);
            m_ioBuffer = nullptr;
            emit error("Nie można utworzyć kontekstu I/O");
            return;
        }
    }

    ~ImageDecoder() {
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

        // Zwolnij wszystkie zasoby
        releaseResources();

        // Zresetuj pozycję odczytu
        m_readPosition = 0;

        // Przygotowanie buforów I/O jeśli zostały zwolnione
        if (!m_ioBuffer) {
            m_ioBuffer = (unsigned char*)av_malloc(m_imageData.size() + AV_INPUT_BUFFER_PADDING_SIZE);
            if (!m_ioBuffer) {
                emit error("Nie można zaalokować pamięci dla danych obrazu");
                return false;
            }
            memcpy(m_ioBuffer, m_imageData.constData(), m_imageData.size());
        }

        if (!m_ioContext) {
            m_ioContext = avio_alloc_context(
                m_ioBuffer, m_imageData.size(), 0, this,
                &ImageDecoder::readPacket, nullptr, &ImageDecoder::seekPacket
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

    QImage decode() {
        if (!initialize()) {
            return QImage();
        }

        AVPacket packet;
        QImage result;

        // Odczytaj pierwszy pakiet
        if (av_read_frame(m_formatContext, &packet) >= 0) {
            if (packet.stream_index == m_imageStream) {
                // Wyślij pakiet do dekodera
                avcodec_send_packet(m_codecContext, &packet);
                
                // Otrzymaj zdekodowaną ramkę
                if (avcodec_receive_frame(m_codecContext, m_frame) == 0) {
                    // Konwertuj ramkę do RGB(A)
                    sws_scale(
                        m_swsContext,
                        (const uint8_t* const*)m_frame->data, m_frame->linesize, 0, m_codecContext->height,
                        m_frameRGB->data, m_frameRGB->linesize
                    );

                    // Sprawdź, czy obraz ma kanał alfa
                    bool hasAlpha = m_codecContext->pix_fmt == AV_PIX_FMT_RGBA || 
                                   m_codecContext->pix_fmt == AV_PIX_FMT_YUVA420P;
                    
                    // Utwórz QImage w odpowiednim formacie
                    if (hasAlpha) {
                        result = QImage(
                            m_frameRGB->data[0],
                            m_codecContext->width,
                            m_codecContext->height,
                            m_frameRGB->linesize[0],
                            QImage::Format_RGBA8888
                        );
                    } else {
                        result = QImage(
                            m_frameRGB->data[0],
                            m_codecContext->width,
                            m_codecContext->height,
                            m_frameRGB->linesize[0],
                            QImage::Format_RGB888
                        );
                    }
                    
                    // Utwórz kopię obrazu, żeby był niezależny od FFmpeg
                    result = result.copy();
                }
            }
            av_packet_unref(&packet);
        }

        // Emituj sygnał z informacją o obrazie
        if (!result.isNull()) {
            emit imageInfo(
                result.width(),
                result.height(),
                result.hasAlphaChannel()
            );
            emit imageReady(result);
        }

        return result;
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

        // Otwórz strumień obrazu
        if (avformat_open_input(&m_formatContext, "", nullptr, nullptr) < 0) {
            emit error("Nie można otworzyć strumienia obrazu");
            return false;
        }

        // Znajdź informacje o strumieniu
        if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
            emit error("Nie można znaleźć informacji o strumieniu");
            return false;
        }

        // Znajdź strumień wideo (obraz)
        m_imageStream = -1;
        for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
            if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                m_imageStream = i;
                break;
            }
        }

        if (m_imageStream == -1) {
            emit error("Nie znaleziono strumienia obrazu");
            return false;
        }

        // Znajdź dekoder
        const AVCodec* codec = avcodec_find_decoder(m_formatContext->streams[m_imageStream]->codecpar->codec_id);
        if (!codec) {
            emit error("Nie znaleziono dekodera dla tego formatu obrazu");
            return false;
        }

        // Utwórz kontekst kodeka
        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) {
            emit error("Nie można utworzyć kontekstu kodeka");
            return false;
        }

        // Skopiuj parametry kodeka
        if (avcodec_parameters_to_context(m_codecContext, m_formatContext->streams[m_imageStream]->codecpar) < 0) {
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
            emit error("Nie można zaalokować ramek obrazu");
            return false;
        }

        // Sprawdź czy obraz ma kanał alfa
        bool hasAlpha = m_codecContext->pix_fmt == AV_PIX_FMT_RGBA || 
                      m_codecContext->pix_fmt == AV_PIX_FMT_YUVA420P;
        
        AVPixelFormat outFormat = hasAlpha ? AV_PIX_FMT_RGBA : AV_PIX_FMT_RGB24;
        
        // Określ rozmiar bufora dla skonwertowanych ramek
        m_bufferSize = av_image_get_buffer_size(
            outFormat,
            m_codecContext->width,
            m_codecContext->height,
            1
        );

        m_buffer = (uint8_t*)av_malloc(m_bufferSize);
        if (!m_buffer) {
            emit error("Nie można zaalokować bufora obrazu");
            return false;
        }

        // Ustaw dane wyjściowe w frameRGB
        av_image_fill_arrays(
            m_frameRGB->data, m_frameRGB->linesize, m_buffer,
            outFormat, m_codecContext->width, m_codecContext->height, 1
        );

        // Utwórz kontekst konwersji
        m_swsContext = sws_getContext(
            m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
            m_codecContext->width, m_codecContext->height, outFormat,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (!m_swsContext) {
            emit error("Nie można utworzyć kontekstu konwersji");
            return false;
        }

        m_initialized = true;
        return true;
    }

signals:
    void imageReady(const QImage& image);
    void error(const QString& message);
    void imageInfo(int width, int height, bool hasAlpha);

private:
    // Funkcje callback dla custom I/O
    static int readPacket(void* opaque, uint8_t* buf, int buf_size) {
        ImageDecoder* decoder = static_cast<ImageDecoder*>(opaque);
        int size = qMin(buf_size, decoder->m_readPosition >= decoder->m_imageData.size() ?
            0 : decoder->m_imageData.size() - decoder->m_readPosition);

        if (size <= 0)
            return AVERROR_EOF;

        memcpy(buf, decoder->m_imageData.constData() + decoder->m_readPosition, size);
        decoder->m_readPosition += size;
        return size;
    }

    static int64_t seekPacket(void* opaque, int64_t offset, int whence) {
        ImageDecoder* decoder = static_cast<ImageDecoder*>(opaque);

        switch (whence) {
            case SEEK_SET:
                decoder->m_readPosition = offset;
                break;
            case SEEK_CUR:
                decoder->m_readPosition += offset;
                break;
            case SEEK_END:
                decoder->m_readPosition = decoder->m_imageData.size() + offset;
                break;
            case AVSEEK_SIZE:
                return decoder->m_imageData.size();
            default:
                return -1;
        }

        return decoder->m_readPosition;
    }

    QByteArray m_imageData;
    int m_readPosition = 0;

    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    SwsContext* m_swsContext = nullptr;
    AVFrame* m_frame = nullptr;
    AVFrame* m_frameRGB = nullptr;
    AVIOContext* m_ioContext = nullptr;
    unsigned char* m_ioBuffer = nullptr;
    int m_imageStream = -1;
    uint8_t* m_buffer = nullptr;
    int m_bufferSize = 0;

    QMutex m_mutex;
    bool m_initialized = false;
};

#endif // IMAGE_DECODER_H