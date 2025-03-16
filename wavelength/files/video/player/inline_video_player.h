#ifndef INLINE_VIDEO_PLAYER_H
#define INLINE_VIDEO_PLAYER_H
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QToolButton>

#include "../decoder/video_decoder.h"


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

        // Obszar wyświetlania wideo - zmniejszamy rozmiar początkowy
        m_videoLabel = new QLabel(this);
        m_videoLabel->setAlignment(Qt::AlignCenter);
        m_videoLabel->setMinimumSize(320, 180); // Mniejszy rozmiar wyjściowy
        // m_videoLabel->setMaximumHeight(240); // Ograniczamy maksymalną wysokość
        m_videoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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

        layout->addLayout(controlLayout);

        // Dekoder wideo
        m_decoder = new VideoDecoder(videoData, this);

        // Połącz sygnały
        connect(m_playButton, &QPushButton::clicked, this, &InlineVideoPlayer::togglePlayback);
        connect(m_progressSlider, &QSlider::sliderMoved, this, &InlineVideoPlayer::seekVideo);
        connect(m_decoder, &VideoDecoder::frameReady, this, &InlineVideoPlayer::updateFrame);
        connect(m_decoder, &VideoDecoder::error, this, &InlineVideoPlayer::handleError);
        connect(m_decoder, &VideoDecoder::videoInfo, this, &InlineVideoPlayer::handleVideoInfo);
        connect(m_decoder, &VideoDecoder::playbackFinished, this, [this]() {
            m_decoder->pause(); // Zatrzymaj odtwarzanie
            m_playButton->setText("▶");
        });

        // Dodajemy obsługę kontroli dźwięku
        connect(m_volumeSlider, &QSlider::valueChanged, this, &InlineVideoPlayer::adjustVolume);
        connect(m_volumeButton, &QToolButton::clicked, this, &InlineVideoPlayer::toggleMute);

        // Nowe połączenie dla aktualizacji pozycji
        connect(m_decoder, &VideoDecoder::positionChanged, this, &InlineVideoPlayer::updateSliderPosition);

        // Inicjalizuj odtwarzacz w osobnym wątku
        QTimer::singleShot(100, this, [this]() {
            m_decoder->start(QThread::HighPriority); // Wyższy priorytet dla lepszej płynności
        });

        // Timer do aktualizacji pozycji suwaka - zwiększona częstotliwość
        m_updateTimer = new QTimer(this);
        m_updateTimer->setInterval(100); // Częstsze aktualizacje
        connect(m_updateTimer, &QTimer::timeout, this, &InlineVideoPlayer::updateProgressUI);
        m_updateTimer->start();
    }

    void updateProgressUI() {
        // Timer do częstszej aktualizacji interfejsu, ale nie modyfikuje pozycji,
        // która jest aktualizowana bezpośrednio przez sygnał positionChanged
    }

    ~InlineVideoPlayer() {
        if (m_decoder) {
            m_decoder->stop();
            m_decoder->wait();
            delete m_decoder;
        }
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

        // Natychmiast aktualizuj widoczny timestamp
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
    QToolButton* m_volumeButton;
    QSlider* m_volumeSlider;
    VideoDecoder* m_decoder;
    QTimer* m_updateTimer;

    QByteArray m_videoData;
    QString m_mimeType;

    int m_videoWidth = 0;
    int m_videoHeight = 0;
    double m_videoDuration = 0;
    double m_currentFramePosition = 0;
    bool m_sliderDragging = false;
    int m_lastVolume = 100; // Domyślny poziom głośności
};



#endif //INLINE_VIDEO_PLAYER_H
