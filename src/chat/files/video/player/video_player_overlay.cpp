//
// Created by szymo on 18.03.2025.
//

#include "video_player_overlay.h"

VideoPlayerOverlay::VideoPlayerOverlay(const QByteArray &videoData, const QString &mimeType, QWidget *parent): QDialog(parent), m_videoData(videoData), m_mimeType(mimeType),
    m_scanlineOpacity(0.15), m_gridOpacity(0.1) {
    setWindowTitle("SYS> WAVELENGTH_VISUAL_STREAM_DECODER");
    setMinimumSize(900, 630);
    setModal(false);

    // Styl całego okna
    setStyleSheet(
        "QDialog {"
        "  background-color: #0a1520;"  // Ciemniejszy granatowy
        "  color: #00ccff;"             // Neonowy niebieski
        "  font-family: 'Consolas';"    // Czcionka technologiczna
        "}"
    );

#ifdef Q_OS_WINDOWS
    // Ustaw ciemny pasek tytułu dla Windows 10/11
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10) {
        HWND hwnd = (HWND)this->winId(); // Pobierz uchwyt okna dialogowego
        if (hwnd) {
            // Użyj atrybutu 20 dla Windows 10 1809+ / Windows 11
            BOOL darkMode = TRUE;
            ::DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));
        }
    }
#endif

    // Główny layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Panel górny z informacjami
    QWidget* topPanel = new QWidget(this);
    QHBoxLayout* topLayout = new QHBoxLayout(topPanel);
    topLayout->setContentsMargins(5, 5, 5, 5);

    // Lewy panel z tytułem
    QLabel* titleLabel = new QLabel("VISUAL STREAM DECODER v2.5", this);
    titleLabel->setStyleSheet(
        "color: #00ffff;"
        "background-color: #001822;"
        "border: 1px solid #00aaff;"
        "padding: 4px;"
        "border-radius: 0px;"
        "font-weight: bold;"
    );

    // Prawy panel ze statusem
    m_statusLabel = new QLabel("INITIALIZING...", this);
    m_statusLabel->setStyleSheet(
        "color: #ffcc00;"
        "background-color: #221800;"
        "border: 1px solid #ffaa00;"
        "padding: 4px;"
        "border-radius: 0px;"
    );

    topLayout->addWidget(titleLabel);
    topLayout->addWidget(m_statusLabel, 1);

    mainLayout->addWidget(topPanel);

    // Container na wideo z ramką AR
    QWidget* videoContainer = new QWidget(this);
    QVBoxLayout* videoLayout = new QVBoxLayout(videoContainer);
    videoLayout->setContentsMargins(20, 20, 20, 20);

    // Stylizacja kontenera
    videoContainer->setStyleSheet(
        "background-color: rgba(0, 20, 30, 100);"
        "border: 1px solid #00aaff;"
    );

    // Label na wideo
    m_videoLabel = new QLabel(this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setMinimumSize(828, 466);
    m_videoLabel->setText("BUFFER LOADING...");
    m_videoLabel->setStyleSheet(
        "color: #00ffff;"
        "background-color: #000000;"
        "border: 1px solid #005599;"
        "font-weight: bold;"
    );

    videoLayout->addWidget(m_videoLabel);
    mainLayout->addWidget(videoContainer);

    // Panel kontrolny w stylu cyberpunk
    QWidget* controlPanel = new QWidget(this);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel);
    controlLayout->setContentsMargins(5, 5, 5, 5);

    // Cyberpunkowe przyciski
    m_playButton = new CyberPushButton("▶", this);
    m_playButton->setFixedSize(40, 30);

    // Cyberpunkowy slider postępu
    m_progressSlider = new CyberSlider(Qt::Horizontal, this);

    // Etykieta czasu
    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setStyleSheet(
        "color: #00ffff;"
        "background-color: #001822;"
        "border: 1px solid #00aaff;"
        "padding: 4px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
    );
    m_timeLabel->setMinimumWidth(120);

    // Przycisk głośności
    m_volumeButton = new CyberPushButton("🔊", this);
    m_volumeButton->setFixedSize(30, 30);

    // Slider głośności
    m_volumeSlider = new CyberSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(80);

    // Dodanie kontrolek do layoutu
    controlLayout->addWidget(m_playButton);
    controlLayout->addWidget(m_progressSlider, 1);
    controlLayout->addWidget(m_timeLabel);
    controlLayout->addWidget(m_volumeButton);
    controlLayout->addWidget(m_volumeSlider);

    mainLayout->addWidget(controlPanel);

    // Panel dolny z informacjami technicznymi
    QWidget* infoPanel = new QWidget(this);
    QHBoxLayout* infoLayout = new QHBoxLayout(infoPanel);
    infoLayout->setContentsMargins(2, 2, 2, 2);

    // Neonowe etykiety z danymi technicznymi
    m_codecLabel = new QLabel("CODEC: ANALYZING", this);
    m_resolutionLabel = new QLabel("RES: --x--", this);
    m_bitrateLabel = new QLabel("BITRATE: --", this);
    m_fpsLabel = new QLabel("FPS: --", this);

    QLabel* securityLabel = new QLabel(
        QString("SEC: LVL%1").arg(QRandomGenerator::global()->bounded(1, 6)), this);

    QString sessionId = QString("%1-%2")
            .arg(QRandomGenerator::global()->bounded(1000, 9999))
            .arg(QRandomGenerator::global()->bounded(10000, 99999));
    QLabel* sessionLabel = new QLabel(QString("SESS: %1").arg(sessionId), this);

    // Stylizacja etykiet informacyjnych
    QString infoLabelStyle =
            "color: #00ccff;"
            "background-color: transparent;"
            "font-family: 'Consolas';"
            "font-size: 8pt;";

    m_codecLabel->setStyleSheet(infoLabelStyle);
    m_resolutionLabel->setStyleSheet(infoLabelStyle);
    m_bitrateLabel->setStyleSheet(infoLabelStyle);
    m_fpsLabel->setStyleSheet(infoLabelStyle);
    securityLabel->setStyleSheet(infoLabelStyle);
    sessionLabel->setStyleSheet(infoLabelStyle);

    infoLayout->addWidget(m_codecLabel);
    infoLayout->addWidget(m_resolutionLabel);
    infoLayout->addWidget(m_bitrateLabel);
    infoLayout->addWidget(m_fpsLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(securityLabel);
    infoLayout->addWidget(sessionLabel);

    mainLayout->addWidget(infoPanel);

    // Połącz sygnały
    connect(m_playButton, &QPushButton::clicked, this, &VideoPlayerOverlay::togglePlayback);
    connect(m_progressSlider, &QSlider::sliderMoved, this, &VideoPlayerOverlay::updateTimeLabel);
    connect(m_progressSlider, &QSlider::sliderPressed, this, &VideoPlayerOverlay::onSliderPressed);
    connect(m_progressSlider, &QSlider::sliderReleased, this, &VideoPlayerOverlay::onSliderReleased);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &VideoPlayerOverlay::adjustVolume);
    connect(m_volumeButton, &QPushButton::clicked, this, &VideoPlayerOverlay::toggleMute);

    // Timer dla animacji i efektów
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(50);
    connect(m_updateTimer, &QTimer::timeout, this, &VideoPlayerOverlay::updateUI);
    m_updateTimer->start();

    // Timer dla cyfrowego "szumu"
    m_glitchTimer = new QTimer(this);
    m_glitchTimer->setInterval(QRandomGenerator::global()->bounded(2000, 5000));
    connect(m_glitchTimer, &QTimer::timeout, this, &VideoPlayerOverlay::triggerGlitch);
    m_glitchTimer->start();

    // Automatycznie rozpocznij odtwarzanie po utworzeniu
    QTimer::singleShot(800, this, &VideoPlayerOverlay::initializePlayer);
}

VideoPlayerOverlay::~VideoPlayerOverlay() {
    // Najpierw zatrzymaj timery
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    if (m_glitchTimer) {
        m_glitchTimer->stop();
    }

    // Bezpiecznie zwolnij zasoby dekodera
    releaseResources();
}

void VideoPlayerOverlay::releaseResources() {
    // Upewnij się, że dekoder zostanie prawidłowo zatrzymany i odłączony
    if (m_decoder) {
        // Odłącz wszystkie połączenia przed zatrzymaniem
        disconnect(m_decoder.get(), nullptr, this, nullptr);

        // Zatrzymaj dekoder i zaczekaj na zakończenie
        m_decoder->stop();
        m_decoder->wait(1000);

        // Zwolnij zasoby
        m_decoder->releaseResources();

        // Wyraźne resetowanie wskaźnika
        m_decoder.reset();
    }
}

void VideoPlayerOverlay::paintEvent(QPaintEvent *event) {
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Siatka hologramu w tle
    if (m_gridOpacity > 0.01) {
        painter.setPen(QPen(QColor(0, 150, 255, 30 * m_gridOpacity), 1, Qt::DotLine));

        // Poziome linie siatki
        for (int y = 0; y < height(); y += 40) {
            painter.drawLine(0, y, width(), y);
        }

        // Pionowe linie siatki
        for (int x = 0; x < width(); x += 40) {
            painter.drawLine(x, 0, x, height());
        }
    }
}

void VideoPlayerOverlay::closeEvent(QCloseEvent *event) {
    // Zatrzymaj timery przed zamknięciem
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    if (m_glitchTimer) {
        m_glitchTimer->stop();
    }

    // Bezpiecznie zwolnij zasoby
    releaseResources();

    // Zaakceptuj zdarzenie zamknięcia
    event->accept();
}

void VideoPlayerOverlay::initializePlayer() {
    if (!m_decoder) {
        m_statusLabel->setText("INICJALIZACJA DEKODERA...");

        m_decoder = std::make_shared<VideoDecoder>(m_videoData, nullptr);

        // Połącz sygnały z użyciem Qt::DirectConnection dla ostatnich aktualizacji
        connect(m_decoder.get(), &VideoDecoder::frameReady, this, &VideoPlayerOverlay::updateFrame, Qt::DirectConnection);
        connect(m_decoder.get(), &VideoDecoder::error, this, &VideoPlayerOverlay::handleError, Qt::DirectConnection);
        connect(m_decoder.get(), &VideoDecoder::videoInfo, this, &VideoPlayerOverlay::handleVideoInfo, Qt::DirectConnection);
        connect(m_decoder.get(), &VideoDecoder::playbackFinished, this, [this]() {
            m_playbackFinished = true;
            m_playButton->setText("↻");
            m_statusLabel->setText("ODTWARZANIE ZAKOŃCZONE");
        }, Qt::DirectConnection);
        connect(m_decoder.get(), &VideoDecoder::positionChanged, this, &VideoPlayerOverlay::updateSliderPosition, Qt::DirectConnection);

        m_decoder->start(QThread::HighPriority);
        m_playButton->setText("▶");
        m_statusLabel->setText("READY");
    }
}

void VideoPlayerOverlay::togglePlayback() {
    if (!m_decoder) {
        initializePlayer();
        return;
    }

    if (m_playbackFinished) {
        m_decoder->reset(); // Reset przewija do 0 i pauzuje
        m_playbackFinished = false;
        m_playButton->setText("▶"); // Gotowy do startu
        m_statusLabel->setText("GOTOWY / PAUZA");
        // Nie wznawiamy automatycznie po resecie.
        return; // Czekaj na kliknięcie, aby odtworzyć ponownie.
    }

    m_decoder->pause(); // Zmienia stan pauzy

    if (m_decoder->isPaused()) {
        m_playButton->setText("▶");
        m_statusLabel->setText("PAUZA");
    } else { // Jeśli teraz odtwarza
        m_playButton->setText("❚❚");
        m_statusLabel->setText("ODTWARZANIE");
    }


    // Animacja efektów przy zmianie stanu
    QPropertyAnimation* gridAnim = new QPropertyAnimation(this, "gridOpacity");
    gridAnim->setDuration(500);
    gridAnim->setStartValue(m_gridOpacity);
    gridAnim->setEndValue(0.5);
    gridAnim->setKeyValueAt(0.5, 0.7);

    QPropertyAnimation* scanAnim = new QPropertyAnimation(this, "scanlineOpacity");
    scanAnim->setDuration(500);
    scanAnim->setStartValue(m_scanlineOpacity);
    scanAnim->setEndValue(0.15);
    scanAnim->setKeyValueAt(0.5, 0.4);

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
    group->addAnimation(gridAnim);
    group->addAnimation(scanAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void VideoPlayerOverlay::onSliderPressed() {
    m_wasPlaying = !m_decoder->isPaused();
    if (m_wasPlaying) {
        m_decoder->pause();
    }
    m_sliderDragging = true;
    m_statusLabel->setText("WYSZUKIWANIE...");
}

void VideoPlayerOverlay::onSliderReleased() {
    seekVideo(m_progressSlider->value());
    m_sliderDragging = false;
    if (m_wasPlaying) {
        m_decoder->pause();
        m_playButton->setText("❚❚");
        m_statusLabel->setText("ODTWARZANIE");
    } else {
        m_statusLabel->setText("PAUZA");
    }
}

void VideoPlayerOverlay::updateTimeLabel(int position) {
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

void VideoPlayerOverlay::updateSliderPosition(double position) {
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

void VideoPlayerOverlay::seekVideo(int position) const {
    if (!m_decoder || m_videoDuration <= 0)
        return;

    double seekPosition = position / 1000.0;
    m_decoder->seek(seekPosition);
}

void VideoPlayerOverlay::updateFrame(const QImage &frame) {
    // Jeśli to pierwsza klatka, zapisz ją jako miniaturkę
    if (m_thumbnailFrame.isNull()) {
        m_thumbnailFrame = frame.copy();
    }

    // Stały rozmiar obszaru wyświetlania
    const int displayWidth = m_videoLabel->width();
    const int displayHeight = m_videoLabel->height();

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

    // Rysujemy przeskalowaną klatkę na kontenerze
    QPainter painter(&containerImage);
    painter.drawImage(QPoint(xOffset, yOffset), scaledFrame);

    // Dodajemy cyfrowe zniekształcenia (scanlines)
    if (m_scanlineOpacity > 0.05) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 60 * m_scanlineOpacity));

        for (int y = 0; y < containerImage.height(); y += 3) {
            painter.drawRect(0, y, containerImage.width(), 1);
        }
    }

    // Opcjonalnie: dodajemy subtelne cyberpunkowe elementy HUD
    if (m_showHUD) {
        // Rysujemy narożniki HUD-a
        painter.setPen(QPen(QColor(0, 200, 255, 150), 1));
        int cornerSize = 20;

        // Lewy górny
        painter.drawLine(5, 5, 5 + cornerSize, 5);
        painter.drawLine(5, 5, 5, 5 + cornerSize);

        // Prawy górny
        painter.drawLine(containerImage.width() - 5 - cornerSize, 5, containerImage.width() - 5, 5);
        painter.drawLine(containerImage.width() - 5, 5, containerImage.width() - 5, 5 + cornerSize);

        // Prawy dolny
        painter.drawLine(containerImage.width() - 5, containerImage.height() - 5 - cornerSize,
                         containerImage.width() - 5, containerImage.height() - 5);
        painter.drawLine(containerImage.width() - 5 - cornerSize, containerImage.height() - 5,
                         containerImage.width() - 5, containerImage.height() - 5);

        // Lewy dolny
        painter.drawLine(5, containerImage.height() - 5 - cornerSize, 5, containerImage.height() - 5);
        painter.drawLine(5, containerImage.height() - 5, 5 + cornerSize, containerImage.height() - 5);

        // Dodajemy informacje techniczne w rogach
        painter.setFont(QFont("Consolas", 8));

        // Timestamp w prawym górnym rogu
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        painter.drawText(containerImage.width() - 80, 20, timestamp);

        // Wskaźnik klatki w lewym dolnym rogu
        int frameNumber = m_frameCounter % 10000;
        painter.drawText(10, containerImage.height() - 10,
                         QString("FRAME: %1").arg(frameNumber, 4, 10, QChar('0')));

        // Wskaźnik rozmiaru w prawym dolnym rogu
        painter.drawText(containerImage.width() - 120, containerImage.height() - 10,
                         QString("%1x%2").arg(m_videoWidth).arg(m_videoHeight));

        m_frameCounter++;
    }

    // Aktualizujemy losowe "trzaski" w obrazie
    if (m_currentGlitchIntensity > 0) {
        // Dodajemy losowe zakłócenia (glitche)
        painter.setPen(Qt::NoPen);

        for (int i = 0; i < m_currentGlitchIntensity * 20; ++i) {
            int glitchHeight = QRandomGenerator::global()->bounded(1, 5);
            int glitchY = QRandomGenerator::global()->bounded(containerImage.height());
            int glitchX = QRandomGenerator::global()->bounded(containerImage.width());
            int glitchWidth = QRandomGenerator::global()->bounded(20, 100);

            QColor glitchColor(
                QRandomGenerator::global()->bounded(0, 255),
                QRandomGenerator::global()->bounded(0, 255),
                QRandomGenerator::global()->bounded(0, 255),
                150
            );

            painter.setBrush(glitchColor);
            painter.drawRect(glitchX, glitchY, glitchWidth, glitchHeight);
        }

        // Zmniejszamy intensywność glitchy z każdą klatką
        m_currentGlitchIntensity *= 0.9;
        if (m_currentGlitchIntensity < 0.1) {
            m_currentGlitchIntensity = 0;
        }
    }

    // Wyświetlamy finalny obraz
    m_videoLabel->setPixmap(QPixmap::fromImage(containerImage));
}

void VideoPlayerOverlay::updateUI() {
    // Funkcja wywołana przez timer
    static qreal pulse = 0;
    pulse += 0.05;

    // Pulsujący efekt poświaty
    if (m_playbackStarted && !m_playbackFinished && !m_decoder->isPaused()) {
        qreal pulseFactor = 0.05 * sin(pulse);
        setScanlineOpacity(0.15 + pulseFactor);
        setGridOpacity(0.1 + pulseFactor);
    }

    // Aktualizacja statusu
    if (m_decoder && m_playbackStarted && !m_statusLabel->text().startsWith("ERROR")) {
        // Co pewien czas pokazujemy losowe informacje "techniczne"
        int randomUpdate = QRandomGenerator::global()->bounded(100);

        if (randomUpdate < 2 && !m_decoder->isPaused()) {
            m_statusLabel->setText(QString("FRAME BUFFER: %1%").arg(QRandomGenerator::global()->bounded(90, 100)));
        } else if (randomUpdate < 4 && !m_decoder->isPaused()) {
            m_statusLabel->setText(QString("SIGNAL STRENGTH: %1%").arg(QRandomGenerator::global()->bounded(85, 99)));
        }
    }

    // Wizualne odświeżenie okna
    update();
}

void VideoPlayerOverlay::handleError(const QString &message) const {
    qDebug() << "Video decoder error:" << message;
    m_statusLabel->setText("ERROR: " + message);
    m_videoLabel->setText("⚠️ " + message);
}

void VideoPlayerOverlay::handleVideoInfo(int width, int height, double fps, double duration) {
    m_playbackStarted = true;
    m_videoWidth = width;
    m_videoHeight = height;
    m_videoDuration = duration;
    m_videoFps = fps > 0 ? fps : 30; // Domyślnie 30 jeśli nie wykryto

    m_progressSlider->setRange(0, duration * 1000);

    // Pobierz pierwszą klatkę jako miniaturkę
    QTimer::singleShot(100, this, [this]() {
        if (m_decoder && !m_decoder->isFinished()) {
            m_decoder->extractFirstFrame();
        }
    });

    // Aktualizuj informacje z faktycznym FPS
    m_resolutionLabel->setText(QString("RES: %1x%2").arg(width).arg(height));
    m_bitrateLabel->setText(QString("BITRATE: %1k").arg(QRandomGenerator::global()->bounded(800, 2500)));
    m_fpsLabel->setText(QString("FPS: %1").arg(qRound(m_videoFps)));
    m_codecLabel->setText("CODEC: AV1/H.265");

    // Dostosuj timer odświeżania do wykrytego FPS
    if (m_updateTimer && m_videoFps > 0) {
        int interval = qMax(16, qRound(1000 / m_videoFps));
        m_updateTimer->setInterval(interval);
    }

    // Aktualizujemy status
    m_statusLabel->setText("READY");

    // Włączamy HUD po potwierdzeniu informacji o wideo
    m_showHUD = true;
}

void VideoPlayerOverlay::adjustVolume(int volume) {
    if (!m_decoder) return;

    float normalizedVolume = volume / 100.0f;
    m_decoder->setVolume(normalizedVolume);
    updateVolumeIcon(normalizedVolume);
}

void VideoPlayerOverlay::toggleMute() {
    if (!m_decoder) return;

    if (m_volumeSlider->value() > 0) {
        m_lastVolume = m_volumeSlider->value();
        m_volumeSlider->setValue(0);
    } else {
        m_volumeSlider->setValue(m_lastVolume > 0 ? m_lastVolume : 100);
    }
}

void VideoPlayerOverlay::updateVolumeIcon(float volume) const {
    if (volume <= 0.01f) {
        m_volumeButton->setText("🔇");
    } else if (volume < 0.5f) {
        m_volumeButton->setText("🔉");
    } else {
        m_volumeButton->setText("🔊");
    }
}

void VideoPlayerOverlay::triggerGlitch() {
    // Wywołujemy losowe zakłócenia w obrazie
    m_currentGlitchIntensity = QRandomGenerator::global()->bounded(10, 50) / 100.0;

    // Ustawiamy kolejny interwał zakłóceń
    m_glitchTimer->setInterval(QRandomGenerator::global()->bounded(3000, 10000));
}
