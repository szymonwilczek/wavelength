#ifndef VIDEO_PLAYER_OVERLAY_H
#define VIDEO_PLAYER_OVERLAY_H

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QToolButton>
#include <QTimer>
#include <QPainter>
#include <QCloseEvent>

#include "../decoder/video_decoder.h"

// Klasa okna dialogowego odtwarzacza wideo
class VideoPlayerOverlay : public QDialog {
    Q_OBJECT

public:
    VideoPlayerOverlay(const QByteArray& videoData, const QString& mimeType, QWidget* parent = nullptr)
        : QDialog(parent), m_videoData(videoData), m_mimeType(mimeType) {
        
        setWindowTitle("Odtwarzacz wideo");
        setMinimumSize(720, 480);
        setModal(false); // Pozwala na interakcję z głównym oknem
        
        // Styl okna
        setStyleSheet("background-color: #1a1a1a; color: white;");
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        
        // Obszar wyświetlania wideo
        m_videoLabel = new QLabel(this);
        m_videoLabel->setAlignment(Qt::AlignCenter);
        m_videoLabel->setFixedSize(720, 405);
        m_videoLabel->setStyleSheet("background-color: #000000; color: #ffffff;");
        m_videoLabel->setText("Ładowanie wideo...");
        layout->addWidget(m_videoLabel);
        
        // Panel kontrolny
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
        
        m_volumeButton = new QToolButton(this);
        m_volumeButton->setText("🔊");
        m_volumeButton->setFixedSize(20, 20);
        m_volumeButton->setCursor(Qt::PointingHandCursor);
        m_volumeButton->setStyleSheet("QToolButton { background-color: #333; color: white; border-radius: 3px; }");
        
        m_volumeSlider = new QSlider(Qt::Horizontal, this);
        m_volumeSlider->setRange(0, 100);
        m_volumeSlider->setValue(100);
        m_volumeSlider->setFixedWidth(60);
        m_volumeSlider->setStyleSheet("QSlider::groove:horizontal { background: #555; height: 3px; }"
                                     "QSlider::handle:horizontal { background: #85c4ff; width: 8px; margin: -3px 0; border-radius: 2px; }");
        
        controlLayout->addWidget(m_playButton);
        controlLayout->addWidget(m_progressSlider, 1);
        controlLayout->addWidget(m_timeLabel);
        controlLayout->addWidget(m_volumeButton);
        controlLayout->addWidget(m_volumeSlider);
        
        layout->addLayout(controlLayout);
        
        // Połącz sygnały
        connect(m_playButton, &QPushButton::clicked, this, &VideoPlayerOverlay::togglePlayback);
        connect(m_progressSlider, &QSlider::sliderMoved, this, &VideoPlayerOverlay::updateTimeLabel);
        connect(m_progressSlider, &QSlider::sliderPressed, this, &VideoPlayerOverlay::onSliderPressed);
        connect(m_progressSlider, &QSlider::sliderReleased, this, &VideoPlayerOverlay::onSliderReleased);
        connect(m_volumeSlider, &QSlider::valueChanged, this, &VideoPlayerOverlay::adjustVolume);
        connect(m_volumeButton, &QToolButton::clicked, this, &VideoPlayerOverlay::toggleMute);
        
        m_updateTimer = new QTimer(this);
        m_updateTimer->setInterval(100);
        connect(m_updateTimer, &QTimer::timeout, this, &VideoPlayerOverlay::updateProgressUI);
        m_updateTimer->start();
        
        // Automatycznie rozpocznij odtwarzanie po utworzeniu
        QTimer::singleShot(100, this, &VideoPlayerOverlay::initializePlayer);
    }
    
    ~VideoPlayerOverlay() {
        // Najpierw zatrzymaj timer aktualizacji
        if (m_updateTimer) {
            m_updateTimer->stop();
        }

        // Bezpiecznie zwolnij zasoby dekodera
        releaseResources();
    }
    
    void releaseResources() {
        // Upewnij się, że dekoder zostanie prawidłowo zatrzymany i odłączony
        if (m_decoder) {
            // Odłącz wszystkie połączenia przed zatrzymaniem
            disconnect(m_decoder.get(), nullptr, this, nullptr);

            // Zatrzymaj dekoder i zaczekaj na zakończenie
            m_decoder->stop();
            m_decoder->wait(1000); // Dłuższy timeout dla pewności

            // Zwolnij zasoby
            m_decoder->releaseResources();

            // Wyraźne resetowanie wskaźnika
            m_decoder.reset();
        }
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        // Zatrzymaj timer przed zamknięciem
        if (m_updateTimer) {
            m_updateTimer->stop();
        }

        // Bezpiecznie zwolnij zasoby
        releaseResources();

        // Zaakceptuj zdarzenie zamknięcia
        event->accept();
    }

private slots:
    void initializePlayer() {
        if (!m_decoder) {
            m_decoder = std::make_shared<VideoDecoder>(m_videoData, nullptr); // Zmienione z this na nullptr

            // Połącz sygnały z użyciem Qt::DirectConnection dla ostatnich aktualizacji
            connect(m_decoder.get(), &VideoDecoder::frameReady, this, &VideoPlayerOverlay::updateFrame, Qt::DirectConnection);
            connect(m_decoder.get(), &VideoDecoder::error, this, &VideoPlayerOverlay::handleError, Qt::DirectConnection);
            connect(m_decoder.get(), &VideoDecoder::videoInfo, this, &VideoPlayerOverlay::handleVideoInfo, Qt::DirectConnection);
            connect(m_decoder.get(), &VideoDecoder::playbackFinished, this, [this]() {
                m_playbackFinished = true;
                m_playButton->setText("↻");
            }, Qt::DirectConnection);
            connect(m_decoder.get(), &VideoDecoder::positionChanged, this, &VideoPlayerOverlay::updateSliderPosition, Qt::DirectConnection);

            m_decoder->start(QThread::HighPriority);
            m_playButton->setText("❚❚");
        }
    }
    
    void togglePlayback() {
        if (!m_decoder) {
            initializePlayer();
            return;
        }
        
        if (m_playbackFinished) {
            m_decoder->reset();
            m_playbackFinished = false;
            m_playButton->setText("❚❚");
            if (!m_decoder->isPaused()) {
                m_decoder->pause();
            }
            m_decoder->pause(); // Zmienia stan pauzy
        } else {
            if (m_decoder->isPaused()) {
                m_decoder->pause(); // Wznów odtwarzanie
                m_playButton->setText("❚❚");
            } else {
                m_decoder->pause(); // Wstrzymaj odtwarzanie
                m_playButton->setText("▶");
            }
        }
    }
    
    void onSliderPressed() {
        m_wasPlaying = !m_decoder->isPaused();
        if (m_wasPlaying) {
            m_decoder->pause();
        }
        m_sliderDragging = true;
    }
    
    void onSliderReleased() {
        seekVideo(m_progressSlider->value());
        m_sliderDragging = false;
        if (m_wasPlaying) {
            m_decoder->pause();
            m_playButton->setText("⏸");
        }
    }
    
    void updateTimeLabel(int position) {
        if (!m_decoder || m_videoDuration <= 0)
            return;
            
        double seekPosition = position / 1000.0;
        int seconds = int(seekPosition) % 60;
        int minutes = int(seekPosition) / 60;
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
    
    void updateSliderPosition(double position) {
        if (m_videoDuration <= 0)
            return;
            
        m_progressSlider->setValue(position * 1000);
        
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
    
    void seekVideo(int position) {
        if (!m_decoder || m_videoDuration <= 0)
            return;
            
        double seekPosition = position / 1000.0;
        m_decoder->seek(seekPosition);
    }
    
    void updateFrame(const QImage& frame) {
        // Jeśli to pierwsza klatka, zapisz ją jako miniaturkę
        if (m_thumbnailFrame.isNull()) {
            m_thumbnailFrame = frame.copy();
        }
        
        // Stały rozmiar obszaru wyświetlania
        const int displayWidth = 720;
        const int displayHeight = 405;
        
        // Tworzymy nowy obraz o stałym rozmiarze z czarnym tłem
        QImage containerImage(displayWidth, displayHeight, QImage::Format_RGB32);
        containerImage.fill(Qt::black);
        
        // Obliczamy rozmiar skalowanej ramki z zachowaniem proporcji
        QSize targetSize = frame.size();
        targetSize.scale(displayWidth, displayHeight, Qt::KeepAspectRatio);
        
        // Skalujemy oryginalną klatkę
        QImage scaledFrame = frame.scaled(targetSize.width(), targetSize.height(),
                                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
        
        // Obliczamy pozycję do umieszczenia przeskalowanej klatki (wyśrodkowana)
        int xOffset = (displayWidth - scaledFrame.width()) / 2;
        int yOffset = (displayHeight - scaledFrame.height()) / 2;
        
        // Rysujemy przeskalowaną klatkę na czarnym tle
        QPainter painter(&containerImage);
        painter.drawImage(QPoint(xOffset, yOffset), scaledFrame);
        
        // Wyświetlamy pełny obraz (z czarnymi paskami)
        m_videoLabel->setPixmap(QPixmap::fromImage(containerImage));
    }
    
    void updateProgressUI() {
        // Funkcja wywoływana przez timer
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
        
        // Pobierz pierwszą klatkę jako miniaturkę
        QTimer::singleShot(100, this, [this]() {
        if (m_decoder && !m_decoder->isFinished()) {
            m_decoder->extractFirstFrame();
        }
    });
    }
    
    void adjustVolume(int volume) {
        if (!m_decoder) return;
        
        float normalizedVolume = volume / 100.0f;
        m_decoder->setVolume(normalizedVolume);
        updateVolumeIcon(normalizedVolume);
    }
    
    void toggleMute() {
        if (!m_decoder) return;
        
        if (m_volumeSlider->value() > 0) {
            m_lastVolume = m_volumeSlider->value();
            m_volumeSlider->setValue(0);
        } else {
            m_volumeSlider->setValue(m_lastVolume > 0 ? m_lastVolume : 100);
        }
    }
    
    void updateVolumeIcon(float volume) {
        if (volume <= 0.01f) {
            m_volumeButton->setText("🔇");
        } else if (volume < 0.5f) {
            m_volumeButton->setText("🔉");
        } else {
            m_volumeButton->setText("🔊");
        }
    }

private:
    QLabel* m_videoLabel;
    QPushButton* m_playButton;
    QSlider* m_progressSlider;
    QLabel* m_timeLabel;
    QToolButton* m_volumeButton;
    QSlider* m_volumeSlider;
    std::shared_ptr<VideoDecoder> m_decoder;
    QTimer* m_updateTimer;
    
    QByteArray m_videoData;
    QString m_mimeType;
    
    int m_videoWidth = 0;
    int m_videoHeight = 0;
    double m_videoDuration = 0;
    double m_currentFramePosition = 0;
    bool m_sliderDragging = false;
    int m_lastVolume = 100;
    bool m_playbackFinished = false;
    bool m_wasPlaying = false;
    QImage m_thumbnailFrame;
};

#endif //VIDEO_PLAYER_OVERLAY_H