#ifndef INLINE_VIDEO_PLAYER_H
#define INLINE_VIDEO_PLAYER_H
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QToolButton>
#include <memory>
#include <QApplication>
#include <QPainter>

#include "../../audio/player/inline_audio_player.h"
#include "../decoder/video_decoder.h"


// Klasa odtwarzacza wideo zintegrowanego z czatem
class InlineVideoPlayer : public QFrame {
    Q_OBJECT

public:
    InlineVideoPlayer(const QByteArray& videoData, const QString& mimeType, QWidget* parent = nullptr)
        : QFrame(parent), m_videoData(videoData), m_mimeType(mimeType) {

        setFrameStyle(QFrame::StyledPanel);
        setStyleSheet("background-color: #1a1a1a; border: 1px solid #444;");
        setFixedSize(720, 450);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        // Obszar wyświetlania wideo - zmniejszamy rozmiar początkowy
        m_videoLabel = new QLabel(this);
        m_videoLabel->setAlignment(Qt::AlignCenter);
        m_videoLabel->setFixedSize(720, 405); // Mniejszy rozmiar wyjściowy
        // m_videoLabel->setMaximumHeight(240); // Ograniczamy maksymalną wysokość
        // m_videoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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

        // Dodajemy kontrolki dźwięku
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
        connect(m_playButton, &QPushButton::clicked, this, &InlineVideoPlayer::togglePlayback);
        connect(m_progressSlider, &QSlider::sliderMoved, this, &InlineVideoPlayer::updateTimeLabel);
        connect(m_progressSlider, &QSlider::sliderPressed, this, &InlineVideoPlayer::onSliderPressed);
        connect(m_progressSlider, &QSlider::sliderReleased, this, &InlineVideoPlayer::onSliderReleased);

        // Dodajemy obsługę kontroli dźwięku
        connect(m_volumeSlider, &QSlider::valueChanged, this, &InlineVideoPlayer::adjustVolume);
        connect(m_volumeButton, &QToolButton::clicked, this, &InlineVideoPlayer::toggleMute);


        m_updateTimer = new QTimer(this);
        m_updateTimer->setInterval(100); // Częstsze aktualizacje
        connect(m_updateTimer, &QTimer::timeout, this, &InlineVideoPlayer::updateProgressUI);
        m_updateTimer->start();

        connect(qApp, &QApplication::aboutToQuit, this, [this]() {
    if (s_activePlayer == this) {
        s_activePlayer = nullptr;
    }
    releaseResources();
});
    }

    void updateProgressUI() {
        // Timer do częstszej aktualizacji interfejsu, ale nie modyfikuje pozycji,
        // która jest aktualizowana bezpośrednio przez sygnał positionChanged
    }

    static InlineVideoPlayer* getActivePlayer() {
        return s_activePlayer;
    }

    ~InlineVideoPlayer() {
        releaseResources();
    }

    void releaseResources() {
        if (m_decoder) {
            m_decoder->stop();
            m_decoder->wait(500); // Czekaj max 500ms na zakończenie wątku
            m_decoder->releaseResources();
        }

        if (s_activePlayer == this) {
            s_activePlayer = nullptr;
        }
        m_isActive = false;
    }

    void activate() {
        // Jeśli już jest aktywny, nic nie rób
        if (m_isActive) return;

        // Dezaktywuj aktualnie aktywny odtwarzacz wideo
        if (s_activePlayer && s_activePlayer != this) {
            s_activePlayer->deactivate();
        }

        // Dezaktywuj aktywny odtwarzacz audio, jeśli taki istnieje
        if (InlineAudioPlayer::getActivePlayer() != nullptr) {
            InlineAudioPlayer::getActivePlayer()->deactivate();
        }

        // Aktywuj ten odtwarzacz
        m_isActive = true;
        s_activePlayer = this;

        // Upewnij się, że dekoder jest w odpowiednim stanie
        if (!m_decoder->isRunning()) {
            if (!m_decoder->reinitialize()) {
                qDebug() << "Nie udało się zainicjalizować dekodera wideo";
                return;
            }
            m_decoder->start(QThread::HighPriority);
        }
    }

    void deactivate() {
        if (!m_isActive) return;

        // Zatrzymaj odtwarzanie
        if (m_decoder && !m_decoder->isPaused()) {
            m_decoder->pause();  // Pauzuj odtwarzanie
            m_playButton->setText("▶");
        }

        // Wyświetl miniaturkę z czarnymi paskami
        if (!m_thumbnailFrame.isNull()) {
            // Stały rozmiar obszaru wyświetlania
            const int displayWidth = 720;
            const int displayHeight = 405;

            // Tworzymy nowy obraz o stałym rozmiarze z czarnym tłem
            QImage containerImage(displayWidth, displayHeight, QImage::Format_RGB32);
            containerImage.fill(Qt::black);

            // Obliczamy rozmiar skalowanej miniatury z zachowaniem proporcji
            QSize targetSize = m_thumbnailFrame.size();
            targetSize.scale(displayWidth, displayHeight, Qt::KeepAspectRatio);

            // Skalujemy miniaturkę
            QImage scaledThumbnail = m_thumbnailFrame.scaled(targetSize.width(), targetSize.height(),
                                                  Qt::KeepAspectRatio, Qt::SmoothTransformation);

            // Obliczamy pozycję do umieszczenia przeskalowanej miniatury (wyśrodkowana)
            int xOffset = (displayWidth - scaledThumbnail.width()) / 2;
            int yOffset = (displayHeight - scaledThumbnail.height()) / 2;

            // Rysujemy przeskalowaną miniaturkę na czarnym tle
            QPainter painter(&containerImage);
            painter.drawImage(QPoint(xOffset, yOffset), scaledThumbnail);

            // Wyświetlamy pełny obraz (z czarnymi paskami)
            m_videoLabel->setPixmap(QPixmap::fromImage(containerImage));
        }

        // Zwolnij zasoby dekodera
        // releaseResources();

        // Usuń status aktywnego, jeśli to ten odtwarzacz
        if (s_activePlayer == this) {
            s_activePlayer = nullptr;
        }

        m_isActive = false;
    }


    void adjustVolume(int volume) {
        if (!m_decoder) return;

        float normalizedVolume = volume / 100.0f;
        m_decoder->setVolume(normalizedVolume);

        // Aktualizacja ikony głośności
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

private slots:

    void onSliderPressed() {
        // Zapamiętaj stan odtwarzania przed przesunięciem
        m_wasPlaying = !m_decoder->isPaused();

        // Zatrzymaj odtwarzanie na czas przesuwania
        if (m_wasPlaying) {
            m_decoder->pause();
        }

        m_sliderDragging = true;
    }

    void onSliderReleased() {
        // Wykonaj faktyczne przesunięcie
        seekVideo(m_progressSlider->value());

        m_sliderDragging = false;

        // Przywróć odtwarzanie jeśli było aktywne wcześniej
        if (m_wasPlaying) {
            m_decoder->pause(); // Przełącza stan pauzy (wznawia)
            m_playButton->setText("⏸");
        }
    }

    void updateTimeLabel(int position) {
        if (!m_decoder || m_videoDuration <= 0)
            return;

        double seekPosition = position / 1000.0;

        // Aktualizuj tylko etykietę czasu, bez faktycznego przesuwania wideo
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
        // Bezpośrednia aktualizacja pozycji suwaka z dekodera
        if (m_videoDuration <= 0)
            return;

        // Aktualizacja suwaka
        m_progressSlider->setValue(position * 1000);

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

    void seekVideo(int position) {
        if (!m_decoder || m_videoDuration <= 0)
            return;

        double seekPosition = position / 1000.0;
        m_decoder->seek(seekPosition);

    }

    void togglePlayback() {
        if (!m_decoder) {
            // Inicjalizacja dekodera przy pierwszym odtwarzaniu
            m_decoder = std::make_shared<VideoDecoder>(m_videoData, this);
            connect(m_decoder.get(), &VideoDecoder::frameReady, this, &InlineVideoPlayer::updateFrame, Qt::QueuedConnection);
            connect(m_decoder.get(), &VideoDecoder::error, this, &InlineVideoPlayer::handleError);
            connect(m_decoder.get(), &VideoDecoder::videoInfo, this, &InlineVideoPlayer::handleVideoInfo);
            connect(m_decoder.get(), &VideoDecoder::playbackFinished, this, [this]() {
                m_playbackFinished = true;
                m_playButton->setText("↻");
            });
            connect(m_decoder.get(), &VideoDecoder::positionChanged, this, &InlineVideoPlayer::updateSliderPosition);

            // Rozpocznij dekodowanie w tle
            m_decoder->start(QThread::HighPriority);

            // Zmień tekst przycisku na pauzę
            m_playButton->setText("❚❚");
            return;
        }

        // Aktywuj ten odtwarzacz przed odtwarzaniem
        activate();

        if (m_playbackFinished) {
            m_decoder->reset();
            m_playbackFinished = false;
            m_playButton->setText("❚❚");
            // Upewnij się, że dekoder jest w stanie pauzy, aby następne wywołanie rozpoczęło odtwarzanie
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

        // Ustaw rozmiar QLabel odpowiednio do proporcji wideo, ale nie przekraczając maksymalnych wymiarów
        QSize videoSize(width, height);
        if (videoSize.width() > 480 || videoSize.height() > 270) {
            videoSize.scale(480, 270, Qt::KeepAspectRatio);
        }

        // Ustaw preferowany rozmiar dla wyświetlacza wideo, ale nadal z ograniczeniem maksymalnym
        m_videoLabel->setMinimumSize(videoSize);

        m_progressSlider->setRange(0, duration * 1000);

        // Pobierz pierwszą klatkę jako miniaturkę
        QTimer::singleShot(100, this, [this]() {
            m_decoder->extractFirstFrame();
        });
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
    int m_lastVolume = 100; // Domyślny poziom głośności
    bool m_playbackFinished = false;
    bool m_wasPlaying = false;
    bool m_isActive = false;                 // Czy ten odtwarzacz jest aktywny
    QImage m_thumbnailFrame;                 // Przechowuje miniaturkę dla zatrzymanego odtwarzacza
    inline static InlineVideoPlayer* s_activePlayer = nullptr;

};



#endif //INLINE_VIDEO_PLAYER_H
