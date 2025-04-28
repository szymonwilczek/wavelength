#include "inline_audio_player.h"

InlineAudioPlayer::InlineAudioPlayer(const QByteArray &audioData, const QString &mimeType, QWidget *parent): QFrame(parent), m_audioData(audioData), m_mimeType(mimeType),
    m_scanlineOpacity(0.15), m_spectrumIntensity(0.6) {

    // Ustawiamy styl i rozmiar - nadal zachowujemy mały rozmiar
    setFixedSize(480, 120);
    setContentsMargins(0, 0, 0, 0);
    setFrameStyle(QFrame::NoFrame);

    // Główny layout
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 10);
    layout->setSpacing(8);

    // Panel górny z informacjami
    const auto topPanel = new QWidget(this);
    const auto topLayout = new QHBoxLayout(topPanel);
    topLayout->setContentsMargins(3, 3, 3, 3);
    topLayout->setSpacing(5);

    // Obszar tytułu/informacji o pliku audio
    m_audioInfoLabel = new QLabel(this);
    m_audioInfoLabel->setStyleSheet(
        "color: #c080ff;"
        "background-color: rgba(30, 15, 60, 180);"
        "border: 1px solid #9060cc;"
        "padding: 3px 8px;"
        "font-family: 'Consolas';"
        "font-size: 9pt;"
        "font-weight: bold;"
    );
    m_audioInfoLabel->setText("NEURAL AUDIO DECODER");
    topLayout->addWidget(m_audioInfoLabel, 1);

    // Status odtwarzania
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(
        "color: #80c0ff;"
        "background-color: rgba(15, 30, 60, 180);"
        "border: 1px solid #6080cc;"
        "padding: 3px 8px;"
        "font-family: 'Consolas';"
        "font-size: 8pt;"
    );
    m_statusLabel->setText("INICJALIZACJA...");
    topLayout->addWidget(m_statusLabel);

    layout->addWidget(topPanel);

    // Wizualizator spektrum audio - mała sekcja z wizualizacją
    m_spectrumView = new QWidget(this);
    m_spectrumView->setFixedHeight(35);
    m_spectrumView->setStyleSheet("background-color: rgba(20, 15, 40, 120); border: 1px solid #6040aa;");

    // Podłączamy timer aktualizacji spektrum
    m_spectrumTimer = new QTimer(this);
    connect(m_spectrumTimer, &QTimer::timeout, m_spectrumView, QOverload<>::of(&QWidget::update));
    m_spectrumTimer->start(50);

    // Nadpisanie metody paintEvent dla m_spectrumView
    m_spectrumView->installEventFilter(this);

    layout->addWidget(m_spectrumView);

    // Panel kontrolny w stylu cyberpunk
    const auto controlPanel = new QWidget(this);
    const auto controlLayout = new QHBoxLayout(controlPanel);
    controlLayout->setContentsMargins(3, 1, 3, 1);
    controlLayout->setSpacing(8);

    // Cyberpunkowe przyciski
    m_playButton = new CyberAudioButton("▶", this);
    m_playButton->setFixedSize(28, 24);

    // Cyberpunkowy slider postępu
    m_progressSlider = new CyberAudioSlider(Qt::Horizontal, this);

    // Etykieta czasu
    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setStyleSheet(
        "color: #b080ff;"
        "background-color: rgba(25, 15, 40, 180);"
        "border: 1px solid #6040aa;"
        "padding: 2px 4px;"
        "font-family: 'Consolas';"
        "font-size: 8pt;"
    );
    m_timeLabel->setFixedWidth(90);

    // Przycisk głośności
    m_volumeButton = new CyberAudioButton("🔊", this);
    m_volumeButton->setFixedSize(24, 24);

    // Slider głośności
    m_volumeSlider = new CyberAudioSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(60);

    // Dodanie kontrolek do layoutu
    controlLayout->addWidget(m_playButton);
    controlLayout->addWidget(m_progressSlider, 1);
    controlLayout->addWidget(m_timeLabel);
    controlLayout->addWidget(m_volumeButton);
    controlLayout->addWidget(m_volumeSlider);

    layout->addWidget(controlPanel);

    // Dekoder audio
    m_decoder = std::make_shared<AudioDecoder>(audioData, this);

    // Połącz sygnały
    connect(m_playButton, &QPushButton::clicked, this, &InlineAudioPlayer::togglePlayback);
    connect(m_progressSlider, &QSlider::sliderMoved, this, &InlineAudioPlayer::updateTimeLabel);
    connect(m_progressSlider, &QSlider::sliderPressed, this, &InlineAudioPlayer::onSliderPressed);
    connect(m_progressSlider, &QSlider::sliderReleased, this, &InlineAudioPlayer::onSliderReleased);
    connect(m_decoder.get(), &AudioDecoder::error, this, &InlineAudioPlayer::handleError);
    connect(m_decoder.get(), &AudioDecoder::audioInfo, this, &InlineAudioPlayer::handleAudioInfo);
    connect(m_decoder.get(), &AudioDecoder::playbackFinished, this, [this]() {
        m_playbackFinished = true;
        m_playButton->setText("↻");
        m_statusLabel->setText("ZAKOŃCZONO ODTWARZANIE");
        decreaseSpectrumIntensity();
        update();
    });

    // Połączenia dla kontrolek dźwięku
    connect(m_volumeSlider, &QSlider::valueChanged, this, &InlineAudioPlayer::adjustVolume);
    connect(m_volumeButton, &QPushButton::clicked, this, &InlineAudioPlayer::toggleMute);

    // Aktualizacja pozycji
    connect(m_decoder.get(), &AudioDecoder::positionChanged, this, &InlineAudioPlayer::updateSliderPosition);

    // Timer dla animacji interfejsu
    m_uiTimer = new QTimer(this);
    m_uiTimer->setInterval(50);
    connect(m_uiTimer, &QTimer::timeout, this, &InlineAudioPlayer::updateUI);
    m_uiTimer->start();

    // Inicjalizuj odtwarzacz w osobnym wątku
    QTimer::singleShot(100, this, [this]() {
        m_decoder->start(QThread::HighPriority);
    });

    // Zabezpieczenie przy zamknięciu aplikacji
    connect(qApp, &QApplication::aboutToQuit, this, [this]() {
        if (s_activePlayer == this) {
            s_activePlayer = nullptr;
        }
        releaseResources();
    });
}

void InlineAudioPlayer::releaseResources() {
    // Zatrzymujemy timery
    if (m_uiTimer) {
        m_uiTimer->stop();
    }
    if (m_spectrumTimer) {
        m_spectrumTimer->stop();
    }

    if (m_decoder) {
        m_decoder->stop();
        m_decoder->wait();
        m_decoder->releaseResources();
    }

    if (s_activePlayer == this) {
        s_activePlayer = nullptr;
    }
    m_isActive = false;
}

void InlineAudioPlayer::activate() {
    if (m_isActive)
        return;

    // Jeśli istnieje inny aktywny odtwarzacz, deaktywuj go najpierw
    if (s_activePlayer && s_activePlayer != this) {
        s_activePlayer->deactivate();
    }

    // Upewnij się, że dekoder jest w odpowiednim stanie
    if (!m_decoder->isRunning()) {
        if (!m_decoder->reinitialize()) {
            qDebug() << "Nie udało się zainicjalizować dekodera";
            return;
        }
        m_decoder->start(QThread::HighPriority);
    }

    // Ustaw ten odtwarzacz jako aktywny
    s_activePlayer = this;
    m_isActive = true;

    // Animacja aktywacji
    const auto spectrumAnim = new QPropertyAnimation(this, "spectrumIntensity");
    spectrumAnim->setDuration(700);
    spectrumAnim->setStartValue(0.1);
    spectrumAnim->setEndValue(0.6);
    spectrumAnim->setEasingCurve(QEasingCurve::OutQuad);
    spectrumAnim->start(QPropertyAnimation::DeleteWhenStopped);
}

void InlineAudioPlayer::deactivate() {
    if (!m_isActive)
        return;

    // Zatrzymaj odtwarzanie, jeśli trwa
    if (m_decoder && !m_decoder->isPaused()) {
        m_decoder->pause(); // Zatrzymaj odtwarzanie
    }

    // Jeśli ten odtwarzacz jest aktywny globalnie, wyczyść referencję
    if (s_activePlayer == this) {
        s_activePlayer = nullptr;
    }

    // Animacja dezaktywacji
    const auto spectrumAnim = new QPropertyAnimation(this, "spectrumIntensity");
    spectrumAnim->setDuration(500);
    spectrumAnim->setStartValue(m_spectrumIntensity);
    spectrumAnim->setEndValue(0.1);
    spectrumAnim->start(QPropertyAnimation::DeleteWhenStopped);

    m_isActive = false;
}

void InlineAudioPlayer::adjustVolume(const int volume) const {
    if (!m_decoder)
        return;

    const float volumeFloat = volume / 100.0f;
    m_decoder->setVolume(volumeFloat);
    updateVolumeIcon(volumeFloat);
}

void InlineAudioPlayer::toggleMute() {
    if (!m_decoder)
        return;

    if (m_volumeSlider->value() > 0) {
        m_lastVolume = m_volumeSlider->value();
        m_volumeSlider->setValue(0);
    } else {
        m_volumeSlider->setValue(m_lastVolume > 0 ? m_lastVolume : 100);
    }
}

void InlineAudioPlayer::updateVolumeIcon(const float volume) const {
    if (volume <= 0.01f) {
        m_volumeButton->setText("🔈");
    } else if (volume < 0.5f) {
        m_volumeButton->setText("🔉");
    } else {
        m_volumeButton->setText("🔊");
    }
}

void InlineAudioPlayer::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Ramki w stylu AR
    constexpr QColor borderColor(140, 80, 240);
    painter.setPen(QPen(borderColor, 1));

    // Technologiczna ramka
    QPainterPath frame;
    constexpr int clipSize = 15;

    // Górna krawędź
    frame.moveTo(clipSize, 0);
    frame.lineTo(width() - clipSize, 0);

    // Prawy górny róg
    frame.lineTo(width(), clipSize);

    // Prawa krawędź
    frame.lineTo(width(), height() - clipSize);

    // Prawy dolny róg
    frame.lineTo(width() - clipSize, height());

    // Dolna krawędź
    frame.lineTo(clipSize, height());

    // Lewy dolny róg
    frame.lineTo(0, height() - clipSize);

    // Lewa krawędź
    frame.lineTo(0, clipSize);

    // Lewy górny róg
    frame.lineTo(clipSize, 0);

    painter.drawPath(frame);

    // Dane techniczne w rogach
    painter.setPen(borderColor.lighter(120));
    painter.setFont(QFont("Consolas", 7));
}

bool InlineAudioPlayer::eventFilter(QObject *watched, QEvent *event) {
    // Obsługa malowania spektrum audio
    if (watched == m_spectrumView && event->type() == QEvent::Paint) {
        paintSpectrum(m_spectrumView);
        return true;
    }
    return QFrame::eventFilter(watched, event);
}

void InlineAudioPlayer::onSliderPressed() {
    // Zapamiętaj stan odtwarzania przed przesunięciem
    m_wasPlaying = !m_decoder->isPaused();

    // Zatrzymaj odtwarzanie na czas przesuwania
    if (m_wasPlaying) {
        m_decoder->pause();
    }

    m_sliderDragging = true;
    m_statusLabel->setText("WYSZUKIWANIE...");
}

void InlineAudioPlayer::onSliderReleased() {
    // Wykonaj faktyczne przesunięcie
    seekAudio(m_progressSlider->value());

    m_sliderDragging = false;

    // Przywróć odtwarzanie jeśli było aktywne wcześniej
    if (m_wasPlaying) {
        m_decoder->pause(); // Przełącza stan pauzy (wznawia)
        m_playButton->setText("❚❚");
        m_statusLabel->setText("ODTWARZANIE");

        // Aktywujemy wizualizację
        increaseSpectrumIntensity();
    } else {
        m_statusLabel->setText("PAUZA");
    }
}

void InlineAudioPlayer::updateTimeLabel(const int position) {
    if (!m_decoder || m_audioDuration <= 0)
        return;

    m_currentPosition = position / 1000.0; // Milisekundy na sekundy
    const int currentSeconds = static_cast<int>(m_currentPosition);
    const int totalSeconds = static_cast<int>(m_audioDuration);

    m_timeLabel->setText(QString("%1:%2 / %3:%4")
        .arg(currentSeconds / 60, 2, 10, QChar('0'))
        .arg(currentSeconds % 60, 2, 10, QChar('0'))
        .arg(totalSeconds / 60, 2, 10, QChar('0'))
        .arg(totalSeconds % 60, 2, 10, QChar('0')));
}

void InlineAudioPlayer::updateSliderPosition(double position) const {
    // Bezpośrednia aktualizacja pozycji suwaka z dekodera
    if (m_audioDuration <= 0)
        return;

    // Zabezpieczenie przed niepoprawnymi wartościami
    if (position < 0) position = 0;
    if (position > m_audioDuration) position = m_audioDuration;

    // Aktualizacja suwaka - bez emitowania sygnałów
    m_progressSlider->blockSignals(true);
    m_progressSlider->setValue(position * 1000);
    m_progressSlider->blockSignals(false);

    // Aktualizacja etykiety czasu
    const int seconds = static_cast<int>(position) % 60;
    const int minutes = static_cast<int>(position) / 60;

    const int totalSeconds = static_cast<int>(m_audioDuration) % 60;
    const int totalMinutes = static_cast<int>(m_audioDuration) / 60;

    m_timeLabel->setText(
        QString("%1:%2 / %3:%4")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(totalMinutes, 2, 10, QChar('0'))
        .arg(totalSeconds, 2, 10, QChar('0'))
    );
}

void InlineAudioPlayer::seekAudio(const int position) const {
    if (!m_decoder || m_audioDuration <= 0)
        return;

    const double seekPos = position / 1000.0; // Milisekundy na sekundy
    m_decoder->seek(seekPos);
}

void InlineAudioPlayer::togglePlayback() {
    if (!m_decoder)
        return;

    // Aktywuj ten odtwarzacz przed odtwarzaniem
    activate();

    if (m_playbackFinished) {
        // Resetuj odtwarzacz
        m_decoder->reset();
        m_playbackFinished = false;
        m_playButton->setText("❚❚");
        m_statusLabel->setText("ODTWARZANIE");
        m_currentPosition = 0;
        updateTimeLabel(0);
        increaseSpectrumIntensity();
        m_decoder->pause(); // Rozpocznij odtwarzanie (przełącz stan)
    } else {
        if (m_decoder->isPaused()) {
            m_playButton->setText("❚❚");
            m_statusLabel->setText("ODTWARZANIE");
            increaseSpectrumIntensity();
            m_decoder->pause(); // Wznów odtwarzanie
        } else {
            m_playButton->setText("▶");
            m_statusLabel->setText("PAUZA");
            decreaseSpectrumIntensity();
            m_decoder->pause(); // Wstrzymaj odtwarzanie
        }
    }
}

void InlineAudioPlayer::handleError(const QString &message) const {
    qDebug() << "Audio decoder error:" << message;
    m_statusLabel->setText("ERROR: " + message.left(20));
    m_audioInfoLabel->setText("⚠️ " + message.left(30));
}

void InlineAudioPlayer::handleAudioInfo(const int sampleRate, const int channels, const double duration) {
    m_audioDuration = duration;
    m_progressSlider->setRange(0, duration * 1000);

    // Wyświetl informacje o audio
    QString audioInfo = m_mimeType;
    if (audioInfo.isEmpty()) {
        audioInfo = "Audio";
    }

    // Usuwamy "audio/" z początku typu MIME jeśli istnieje
    audioInfo.replace("audio/", "");

    // Dodajemy informacje o parametrach audio
    audioInfo += QString(" (%1kHz, %2ch)").arg(sampleRate/1000.0, 0, 'f', 1).arg(channels);
    m_audioInfoLabel->setText(audioInfo.toUpper());
    m_statusLabel->setText("GOTOWY");

    // Przygotowujemy losowe dane dla wizualizacji spektrum
    for (int i = 0; i < 64; i++) {
        m_spectrumData.append(0.2 + QRandomGenerator::global()->bounded(60) / 100.0);
    }
}

void InlineAudioPlayer::updateUI() {
    // Aktualizacja stanu UI i animacje
    if (m_decoder && !m_decoder->isPaused() && !m_playbackFinished) {
        // Co pewien czas aktualizacja statusu
        const int randomUpdate = QRandomGenerator::global()->bounded(100);
        if (randomUpdate < 2) {
            m_statusLabel->setText(QString("BUFFER: %1%").arg(QRandomGenerator::global()->bounded(95, 100)));
        } else if (randomUpdate < 4) {
            m_statusLabel->setText(QString("SYNC: %1%").arg(QRandomGenerator::global()->bounded(98, 100)));
        }

        // Aktualizacja danych spektrum - losowe fluktuacje dla efektów wizualnych
        for (int i = 0; i < m_spectrumData.size(); i++) {
            if (!m_decoder->isPaused()) {
                // Bardziej dynamiczne zmiany podczas odtwarzania
                const double change = (QRandomGenerator::global()->bounded(100) - 50) / 250.0;
                m_spectrumData[i] += change;
                m_spectrumData[i] = qBound(0.05, m_spectrumData[i], 1.0);
            } else {
                // Opadające słupki podczas pauzy
                if (m_spectrumData[i] > 0.2) {
                    m_spectrumData[i] *= 0.98;
                }
            }
        }
    }
}

void InlineAudioPlayer::paintSpectrum(QWidget *target) {
    QPainter painter(target);
    painter.setRenderHint(QPainter::Antialiasing);

    // Wypełniamy tło
    painter.fillRect(target->rect(), QColor(10, 5, 20));

    // Rysujemy siatkę
    painter.setPen(QPen(QColor(50, 30, 90, 100), 1, Qt::DotLine));

    const int stepX = target->width() / 8;
    for (int x = stepX; x < target->width(); x += stepX) {
        painter.drawLine(x, 0, x, target->height());
    }

    const int stepY = target->height() / 3;
    for (int y = stepY; y < target->height(); y += stepY) {
        painter.drawLine(0, y, target->width(), y);
    }

    // Rysujemy słupki spektrum
    if (!m_spectrumData.isEmpty()) {
        const int barCount = qMin(32, m_spectrumData.size());
        const double barWidth = static_cast<double>(target->width()) / barCount;

        for (int i = 0; i < barCount; i++) {
            // Wysokość słupka zależna od danych i intensywności
            const double height = m_spectrumData[i] * m_spectrumIntensity * target->height() * 0.9;

            // Gradient dla słupka - od jasnego po ciemny
            QLinearGradient gradient(0, target->height() - height, 0, target->height());
            gradient.setColorAt(0, QColor(170, 100, 255, 200));
            gradient.setColorAt(0.5, QColor(120, 80, 200, 150));
            gradient.setColorAt(1, QColor(60, 40, 120, 100));

            // Rysujemy słupek
            QRectF barRect(i * barWidth, target->height() - height, barWidth - 1, height);
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(barRect, 1, 1);

            // Dodajemy "błysk" na górze słupka
            if (height > 3) {
                painter.setBrush(QColor(255, 255, 255, 150));
                painter.drawRoundedRect(QRectF(i * barWidth, target->height() - height, barWidth - 1, 2), 1, 1);
            }
        }
    }
}
