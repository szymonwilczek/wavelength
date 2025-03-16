#ifndef WAVELENGTH_TEXT_DISPLAY_H
#define WAVELENGTH_TEXT_DISPLAY_H

#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QBuffer>
#include <QByteArray>
#include <QMap>
#include <QUuid>
#include <QScrollBar>
#include <QTimer>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include <QMessageBox>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

// Klasa dekodująca wideo w osobnym wątku
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
        for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
            if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                m_videoStream = i;
                break;
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

protected:
    void run() override {
        if (!initialize()) {
            return;
        }

        AVPacket packet;
        bool frameFinished;

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

                    // Prosty throttling dla ograniczenia FPS
                    QThread::msleep(40); // ~25fps
                }
            }

            av_packet_unref(&packet);
        }
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

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    bool m_stopped;
    bool m_paused = true;
    bool m_seeking = false;
    int64_t m_seekPosition = 0;
};

// Klasa odtwarzacza wideo zintegrowanego z czatem
class InlineVideoPlayer : public QFrame {
    Q_OBJECT

public:
    InlineVideoPlayer(const QByteArray& videoData, const QString& mimeType, QWidget* parent = nullptr)
        : QFrame(parent), m_videoData(videoData), m_mimeType(mimeType) {

        setFrameStyle(QFrame::StyledPanel);
        setStyleSheet("background-color: #1a1a1a; border: 1px solid #444;");

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        // Obszar wyświetlania wideo
        m_videoLabel = new QLabel(this);
        m_videoLabel->setAlignment(Qt::AlignCenter);
        m_videoLabel->setMinimumSize(320, 240);
        m_videoLabel->setStyleSheet("background-color: #000000; color: #ffffff;");
        m_videoLabel->setText("Ładowanie wideo...");
        layout->addWidget(m_videoLabel);

        // Kontrolki odtwarzania
        QHBoxLayout* controlLayout = new QHBoxLayout();
        controlLayout->setContentsMargins(5, 0, 5, 5);

        m_playButton = new QPushButton("▶", this);
        m_playButton->setFixedSize(30, 20);
        m_playButton->setCursor(Qt::PointingHandCursor);
        m_playButton->setStyleSheet("QPushButton { background-color: #333; color: white; border-radius: 3px; }");

        m_progressSlider = new QSlider(Qt::Horizontal, this);
        m_progressSlider->setStyleSheet("QSlider::groove:horizontal { background: #555; height: 5px; }"
                                      "QSlider::handle:horizontal { background: #85c4ff; width: 10px; margin: -3px 0; border-radius: 3px; }");

        m_timeLabel = new QLabel("00:00 / 00:00", this);
        m_timeLabel->setStyleSheet("color: #aaa; font-size: 8pt; min-width: 80px;");

        controlLayout->addWidget(m_playButton);
        controlLayout->addWidget(m_progressSlider, 1);
        controlLayout->addWidget(m_timeLabel);

        layout->addLayout(controlLayout);

        // Dekoder wideo
        m_decoder = new VideoDecoder(videoData, this);

        // Połącz sygnały
        connect(m_playButton, &QPushButton::clicked, this, &InlineVideoPlayer::togglePlayback);
        connect(m_progressSlider, &QSlider::sliderPressed, this, [this]() {
            m_sliderDragging = true;
        });
        connect(m_progressSlider, &QSlider::sliderReleased, this, [this]() {
            m_sliderDragging = false;
            m_decoder->seek(m_progressSlider->value() / 1000.0);
        });
        connect(m_decoder, &VideoDecoder::frameReady, this, &InlineVideoPlayer::updateFrame);
        connect(m_decoder, &VideoDecoder::error, this, &InlineVideoPlayer::handleError);
        connect(m_decoder, &VideoDecoder::videoInfo, this, &InlineVideoPlayer::handleVideoInfo);
        connect(m_decoder, &VideoDecoder::playbackFinished, this, [this]() {
            m_decoder->pause(); // Zatrzymaj odtwarzanie
            m_playButton->setText("▶");
        });

        // Inicjalizuj odtwarzacz w osobnym wątku
        QTimer::singleShot(100, this, [this]() {
            m_decoder->start(QThread::LowPriority);
        });

        // Timer do aktualizacji pozycji suwaka
        m_updateTimer = new QTimer(this);
        m_updateTimer->setInterval(500);
        connect(m_updateTimer, &QTimer::timeout, this, &InlineVideoPlayer::updatePosition);
        m_updateTimer->start();
    }

    ~InlineVideoPlayer() {
        if (m_decoder) {
            m_decoder->stop();
            m_decoder->wait();
            delete m_decoder;
        }
    }

private slots:
    void togglePlayback() {
        if (!m_decoder)
            return;

        m_decoder->pause();

        if (m_decoder->isPaused()) {
            m_playButton->setText("▶");
        } else {
            m_playButton->setText("⏸");
        }
    }

    void updateFrame(const QImage& frame) {
        if (!frame.isNull()) {
            // Skaluj obraz do rozmiaru etykiety z zachowaniem proporcji
            QSize labelSize = m_videoLabel->size();
            QPixmap pixmap = QPixmap::fromImage(frame).scaled(
                labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation
            );

            m_videoLabel->setPixmap(pixmap);
            m_currentFramePosition += 1.0 / 25.0; // Przybliżone 25 fps
        }
    }

    void updatePosition() {
        if (!m_sliderDragging && m_videoDuration > 0) {
            double position = m_currentFramePosition;
            int sliderPos = position * 1000;
            m_progressSlider->setValue(sliderPos);

            // Aktualizacja etykiety czasu
            int seconds = int(position) % 60;
            int minutes = int(position) / 60;

            int totalSeconds = int(m_videoDuration) % 60;
            int totalMinutes = int(m_videoDuration) / 60;

            m_timeLabel->setText(
                QString("%1:%2 / %3:%4")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(totalMinutes, 2, 10, QChar('0'))
                .arg(totalSeconds, 2, 10, QChar('0'))
            );
        }
    }

    void handleError(const QString& message) {
        qDebug() << "Video decoder error:" << message;
        m_videoLabel->setText("⚠️ " + message);
    }

    void handleVideoInfo(int width, int height, double duration) {
        m_videoWidth = width;
        m_videoHeight = height;
        m_videoDuration = duration;

        m_progressSlider->setRange(0, duration * 1000);

        // Wyświetl pierwszą klatkę
        m_decoder->seek(0);
    }

private:
    QLabel* m_videoLabel;
    QPushButton* m_playButton;
    QSlider* m_progressSlider;
    QLabel* m_timeLabel;
    VideoDecoder* m_decoder;
    QTimer* m_updateTimer;

    QByteArray m_videoData;
    QString m_mimeType;

    int m_videoWidth = 0;
    int m_videoHeight = 0;
    double m_videoDuration = 0;
    double m_currentFramePosition = 0;
    bool m_sliderDragging = false;
};

// Główna klasa wyświetlająca czat
class WavelengthTextDisplay : public QWidget {
    Q_OBJECT

public:
    WavelengthTextDisplay(QWidget* parent = nullptr) : QWidget(parent) {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // Obszar przewijania
        m_scrollArea = new QScrollArea(this);
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_scrollArea->setStyleSheet("QScrollArea { background-color: #232323; border: 1px solid #3a3a3a; border-radius: 5px; }");

        // Widget zawartości
        m_contentWidget = new QWidget(m_scrollArea);
        m_contentLayout = new QVBoxLayout(m_contentWidget);
        m_contentLayout->setAlignment(Qt::AlignTop);
        m_contentLayout->setSpacing(5);
        m_contentLayout->setContentsMargins(10, 10, 10, 10);

        m_scrollArea->setWidget(m_contentWidget);
        mainLayout->addWidget(m_scrollArea);
    }

    void appendMessage(const QString& formattedMessage) {
        qDebug() << "Appending message:" << formattedMessage.left(50) << "...";

        // Sprawdzamy czy wiadomość zawiera wideo
        if (formattedMessage.contains("video-placeholder")) {
            processMessageWithVideo(formattedMessage);
        } else {
            // Standardowa wiadomość tekstowa
            QLabel* messageLabel = new QLabel(formattedMessage, m_contentWidget);
            messageLabel->setTextFormat(Qt::RichText);
            messageLabel->setWordWrap(true);
            messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            messageLabel->setOpenExternalLinks(true);

            m_contentLayout->addWidget(messageLabel);
        }

        // Przewiń do najnowszej wiadomości
        QTimer::singleShot(0, this, &WavelengthTextDisplay::scrollToBottom);
    }

    void processMessageWithVideo(const QString& formattedMessage) {
        // Wyodrębnij podstawowy tekst wiadomości (bez znacznika wideo)
        QString messageText = formattedMessage;
        int videoPos = messageText.indexOf("<div class='video-placeholder'");
        if (videoPos > 0) {
            messageText = messageText.left(videoPos);

            // Dodaj tekst wiadomości, jeśli istnieje
            if (!messageText.trimmed().isEmpty()) {
                QLabel* messageLabel = new QLabel(messageText, m_contentWidget);
                messageLabel->setTextFormat(Qt::RichText);
                messageLabel->setWordWrap(true);
                m_contentLayout->addWidget(messageLabel);
            }
        }

        // Wyodrębnij dane wideo z wiadomości
        QRegExp videoRegex("data-mime-type='([^']*)'.*data-base64='([^']*)'.*data-filename='([^']*)'");
        videoRegex.setMinimal(true);

        if (videoRegex.indexIn(formattedMessage) != -1) {
            QString mimeType = videoRegex.cap(1);
            QString base64Data = videoRegex.cap(2);
            QString filename = videoRegex.cap(3);

            qDebug() << "Found video:" << filename << "MIME:" << mimeType;
            qDebug() << "Base64 data length:" << base64Data.length();

            // Dodaj informację o pliku wideo
            QLabel* fileInfoLabel = new QLabel(QString("📹 <span style='color:#aaaaaa; font-size:9pt;'>%1</span>").arg(filename), m_contentWidget);
            fileInfoLabel->setTextFormat(Qt::RichText);
            m_contentLayout->addWidget(fileInfoLabel);

            // Dodaj odtwarzacz wideo
            if (!base64Data.isEmpty() && base64Data.length() > 100) {
                QByteArray videoData = QByteArray::fromBase64(base64Data.toUtf8());

                if (!videoData.isEmpty()) {
                    InlineVideoPlayer* videoPlayer = new InlineVideoPlayer(videoData, mimeType, m_contentWidget);
                    m_contentLayout->addWidget(videoPlayer);
                    qDebug() << "Added inline video player, data size:" << videoData.size();
                } else {
                    qDebug() << "Failed to decode base64 data";
                    QLabel* errorLabel = new QLabel("⚠️ Nie można zdekodować danych wideo", m_contentWidget);
                    errorLabel->setStyleSheet("color: #ff5555;");
                    m_contentLayout->addWidget(errorLabel);
                }
            } else {
                qDebug() << "Invalid video data, not adding player";
                QLabel* errorLabel = new QLabel("⚠️ Nie można wyświetlić wideo (uszkodzone dane)", m_contentWidget);
                errorLabel->setStyleSheet("color: #ff5555;");
                m_contentLayout->addWidget(errorLabel);
            }
        } else {
            qDebug() << "Failed to extract video data";
        }
    }

    void clear() {
        qDebug() << "Clearing all messages";
        QLayoutItem* item;
        while ((item = m_contentLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }

public slots:
    void scrollToBottom() {
        m_scrollArea->verticalScrollBar()->setValue(
            m_scrollArea->verticalScrollBar()->maximum()
        );
    }

private:
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
};

#endif // WAVELENGTH_TEXT_DISPLAY_H