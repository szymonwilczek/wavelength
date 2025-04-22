#ifndef INLINE_AUDIO_PLAYER_H
#define INLINE_AUDIO_PLAYER_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QToolButton>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <memory>
#include <QApplication>
#include <QStyle>

#include "../../audio/decoder/audio_decoder.h"

// Cyberpunkowy suwak dla odtwarzacza audio - wersja mniejsza niż w video_player
class CyberAudioSlider : public QSlider {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberAudioSlider(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QSlider(orientation, parent), m_glowIntensity(0.5) {
        setStyleSheet("background: transparent; border: none;");
    }

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity) {
        m_glowIntensity = intensity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Paleta kolorów cyberpunkowych - odcień fioletowo-niebieski dla audio
        QColor trackColor(40, 30, 80);            // ciemny fiolet
        QColor progressColor(140, 70, 240);       // neonowy fiolet
        QColor handleColor(160, 100, 255);        // jaśniejszy neon
        QColor glowColor(150, 80, 255, 80);       // poświata

        const int handleWidth = 12;
        const int handleHeight = 16;
        const int trackHeight = 3;

        // Rysowanie ścieżki
        QRect trackRect = rect().adjusted(5, (height() - trackHeight) / 2, -5, -(height() - trackHeight) / 2);

        // Ścieżka z zaokrągleniem
        QPainterPath trackPath;
        trackPath.addRoundedRect(trackRect, 1, 1);

        // Cieniowanie dla ścieżki
        painter.setPen(Qt::NoPen);
        painter.setBrush(trackColor);
        painter.drawPath(trackPath);

        // Oblicz pozycję wskaźnika
        int handlePos = QStyle::sliderPositionFromValue(minimum(), maximum(), value(), width() - handleWidth);

        // Rysowanie wypełnionej części
        if (value() > minimum()) {
            QRect progressRect = trackRect;
            progressRect.setWidth(handlePos + handleWidth/2);

            QPainterPath progressPath;
            progressPath.addRoundedRect(progressRect, 1, 1);

            painter.setBrush(progressColor);
            painter.drawPath(progressPath);

            // Kreski w wypełnionej części
            painter.setOpacity(0.4);
            painter.setPen(QPen(QColor(255, 255, 255, 40), 1));

            for (int i = 0; i < progressRect.width(); i += 10) {
                painter.drawLine(i, progressRect.top(), i, progressRect.bottom());
            }
            painter.setOpacity(1.0);
        }

        // Wizualizacja dźwięku - małe słupki pulsujące
        int barCount = 6;
        int maxBarHeight = 8;
        int barWidth = 2;
        int barSpacing = 4;
        int startX = handlePos - ((barCount * (barWidth + barSpacing)) / 2);
        int barY = trackRect.top() - maxBarHeight - 2;

        if (value() > minimum() + (maximum() - minimum()) * 0.05) {
            painter.setPen(Qt::NoPen);

            for (int i = 0; i < barCount; i++) {
                // Generuj wysokość słupka na podstawie pozycji i czasu
                double phase = (QDateTime::currentMSecsSinceEpoch() % 1000) / 1000.0;
                double offset = (double)i / barCount;
                double barHeight = maxBarHeight * (0.3 + 0.7 * qAbs(sin((phase + offset) * M_PI * 3)));

                // Neonowy kolor z przejściem
                QColor barColor;
                if (i < barCount / 2) {
                    barColor = progressColor.lighter(100 + i * 20);
                } else {
                    barColor = progressColor.lighter(100 + (barCount - i) * 20);
                }

                painter.setBrush(barColor);
                painter.drawRect(startX + i * (barWidth + barSpacing), barY + (maxBarHeight - barHeight),
                                barWidth, barHeight);
            }
        }

        // Rysowanie uchwytu z efektem świecenia
        QRect handleRect(handlePos, (height() - handleHeight) / 2, handleWidth, handleHeight);

        // Poświata neonu
        if (m_glowIntensity > 0.2) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(glowColor);

            for (int i = 3; i > 0; i--) {
                double glowSize = i * 2.0 * m_glowIntensity;
                QRect glowRect = handleRect.adjusted(-glowSize, -glowSize, glowSize, glowSize);
                painter.setOpacity(0.15 * m_glowIntensity);
                painter.drawRoundedRect(glowRect, 4, 4);
            }

            painter.setOpacity(1.0);
        }

        // Rysowanie uchwytu
        QPainterPath handlePath;
        handlePath.addRoundedRect(handleRect, 2, 2);

        painter.setPen(QPen(handleColor, 1));
        painter.setBrush(QColor(30, 20, 60));
        painter.drawPath(handlePath);

        // Dodajemy wewnętrzne linie dla efektu technologicznego
        painter.setPen(QPen(handleColor.lighter(), 1));
        painter.drawLine(handleRect.left() + 3, handleRect.top() + 3,
                        handleRect.right() - 3, handleRect.top() + 3);
        painter.drawLine(handleRect.left() + 3, handleRect.bottom() - 3,
                        handleRect.right() - 3, handleRect.bottom() - 3);
    }

    void enterEvent(QEvent* event) override {
        // Animowana poświata przy najechaniu
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(300);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.9);
        anim->start(QPropertyAnimation::DeleteWhenStopped);

        QSlider::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        // Wygaszenie poświaty przy opuszczeniu
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(300);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.5);
        anim->start(QPropertyAnimation::DeleteWhenStopped);

        QSlider::leaveEvent(event);
    }

private:
    double m_glowIntensity;
};

// Cyberpunkowy przycisk audio
class CyberAudioButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberAudioButton(const QString& text, QWidget* parent = nullptr)
        : QPushButton(text, parent), m_glowIntensity(0.5) {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet("background-color: transparent; border: none;");

        // Timer dla subtelnych efektów
        m_pulseTimer = new QTimer(this);
        connect(m_pulseTimer, &QTimer::timeout, this, [this]() {
            double phase = sin(QDateTime::currentMSecsSinceEpoch() * 0.002) * 0.1;
            setGlowIntensity(m_baseGlowIntensity + phase);
        });
        m_pulseTimer->start(50);

        m_baseGlowIntensity = 0.5;
    }

    ~CyberAudioButton() {
        if (m_pulseTimer) {
            m_pulseTimer->stop();
            m_pulseTimer->deleteLater();
        }
    }

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity) {
        m_glowIntensity = intensity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Paleta kolorów - fioletowo-niebieska paleta dla audio
        QColor bgColor(30, 20, 60);       // ciemny tło
        QColor borderColor(140, 80, 240); // neonowy fioletowy brzeg
        QColor textColor(160, 100, 255);  // tekst fioletowy
        QColor glowColor(150, 80, 255, 50); // poświata

        // Ścieżka przycisku - ścięte rogi dla cyberpunkowego stylu
        QPainterPath path;
        int clipSize = 4; // mniejszy rozmiar ścięcia dla mniejszego przycisku

        path.moveTo(clipSize, 0);
        path.lineTo(width() - clipSize, 0);
        path.lineTo(width(), clipSize);
        path.lineTo(width(), height() - clipSize);
        path.lineTo(width() - clipSize, height());
        path.lineTo(clipSize, height());
        path.lineTo(0, height() - clipSize);
        path.lineTo(0, clipSize);
        path.closeSubpath();

        // Efekt poświaty przy hover/pressed
        if (m_glowIntensity > 0.2) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(glowColor);

            for (int i = 3; i > 0; i--) {
                double glowSize = i * 1.5 * m_glowIntensity;
                QPainterPath glowPath;
                glowPath.addRoundedRect(rect().adjusted(-glowSize, -glowSize, glowSize, glowSize), 3, 3);
                painter.setOpacity(0.15 * m_glowIntensity);
                painter.drawPath(glowPath);
            }

            painter.setOpacity(1.0);
        }

        // Tło przycisku
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawPath(path);

        // Obramowanie
        painter.setPen(QPen(borderColor, 1.2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Ozdobne linie wewnętrzne
        painter.setPen(QPen(borderColor.darker(150), 1, Qt::DotLine));
        painter.drawLine(4, 4, width() - 4, 4);
        painter.drawLine(4, height() - 4, width() - 4, height() - 4);

        // Znaczniki w rogach (mniejsze dla audio)
        int markerSize = 2;
        painter.setPen(QPen(borderColor, 1, Qt::SolidLine));

        // Lewy górny marker
        painter.drawLine(clipSize + 1, 2, clipSize + 1 + markerSize, 2);
        painter.drawLine(clipSize + 1, 2, clipSize + 1, 2 + markerSize);

        // Prawy górny marker
        painter.drawLine(width() - clipSize - 1 - markerSize, 2, width() - clipSize - 1, 2);
        painter.drawLine(width() - clipSize - 1, 2, width() - clipSize - 1, 2 + markerSize);

        // Prawy dolny marker
        painter.drawLine(width() - clipSize - 1 - markerSize, height() - 2, width() - clipSize - 1, height() - 2);
        painter.drawLine(width() - clipSize - 1, height() - 2, width() - clipSize - 1, height() - 2 - markerSize);

        // Lewy dolny marker
        painter.drawLine(clipSize + 1, height() - 2, clipSize + 1 + markerSize, height() - 2);
        painter.drawLine(clipSize + 1, height() - 2, clipSize + 1, height() - 2 - markerSize);

        // Tekst przycisku
        painter.setPen(QPen(textColor, 1));
        painter.setFont(QFont("Consolas", 9, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, text());
    }

    void enterEvent(QEvent* event) override {
        m_baseGlowIntensity = 0.8;
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        m_baseGlowIntensity = 0.5;
        QPushButton::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        m_baseGlowIntensity = 1.0;
        QPushButton::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        m_baseGlowIntensity = 0.8;
        QPushButton::mouseReleaseEvent(event);
    }

private:
    double m_glowIntensity;
    double m_baseGlowIntensity;
    QTimer* m_pulseTimer;
};

// Cyberpunkowy odtwarzacz audio
class InlineAudioPlayer : public QFrame {
    Q_OBJECT
    Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
    Q_PROPERTY(double spectrumIntensity READ spectrumIntensity WRITE setSpectrumIntensity)

public:
    static InlineAudioPlayer* getActivePlayer() {
        return s_activePlayer;
    }

    InlineAudioPlayer(const QByteArray& audioData, const QString& mimeType, QWidget* parent = nullptr)
        : QFrame(parent), m_audioData(audioData), m_mimeType(mimeType),
          m_scanlineOpacity(0.15), m_spectrumIntensity(0.6) {

        // Ustawiamy styl i rozmiar - nadal zachowujemy mały rozmiar
        setFixedSize(480, 120);
        setContentsMargins(0, 0, 0, 0);
        setFrameStyle(QFrame::NoFrame);

        // Główny layout
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 10);
        layout->setSpacing(8);

        // Panel górny z informacjami
        QWidget* topPanel = new QWidget(this);
        QHBoxLayout* topLayout = new QHBoxLayout(topPanel);
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
        QWidget* controlPanel = new QWidget(this);
        QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel);
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
            m_spectrumIntensity = 0.1; // Zmniejszenie intensywności spektrum
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

    // Akcesory dla właściwości animacji
    double scanlineOpacity() const { return m_scanlineOpacity; }
    void setScanlineOpacity(double opacity) {
        m_scanlineOpacity = opacity;
        update();
    }

    double spectrumIntensity() const { return m_spectrumIntensity; }
    void setSpectrumIntensity(double intensity) {
        m_spectrumIntensity = intensity;
        update();
    }

    ~InlineAudioPlayer() {
        releaseResources();
    }

    void releaseResources() {
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

    void activate() {
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
        QPropertyAnimation* spectrumAnim = new QPropertyAnimation(this, "spectrumIntensity");
        spectrumAnim->setDuration(700);
        spectrumAnim->setStartValue(0.1);
        spectrumAnim->setEndValue(0.6);
        spectrumAnim->setEasingCurve(QEasingCurve::OutQuad);
        spectrumAnim->start(QPropertyAnimation::DeleteWhenStopped);
    }

    void deactivate() {
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
        QPropertyAnimation* spectrumAnim = new QPropertyAnimation(this, "spectrumIntensity");
        spectrumAnim->setDuration(500);
        spectrumAnim->setStartValue(m_spectrumIntensity);
        spectrumAnim->setEndValue(0.1);
        spectrumAnim->start(QPropertyAnimation::DeleteWhenStopped);

        m_isActive = false;
    }

    void adjustVolume(int volume) {
        if (!m_decoder)
            return;

        float volumeFloat = volume / 100.0f;
        m_decoder->setVolume(volumeFloat);
        updateVolumeIcon(volumeFloat);
    }

    void toggleMute() {
        if (!m_decoder)
            return;

        if (m_volumeSlider->value() > 0) {
            m_lastVolume = m_volumeSlider->value();
            m_volumeSlider->setValue(0);
        } else {
            m_volumeSlider->setValue(m_lastVolume > 0 ? m_lastVolume : 100);
        }
    }

    void updateVolumeIcon(float volume) {
        if (volume <= 0.01f) {
            m_volumeButton->setText("🔈");
        } else if (volume < 0.5f) {
            m_volumeButton->setText("🔉");
        } else {
            m_volumeButton->setText("🔊");
        }
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Ramki w stylu AR
        QColor borderColor(140, 80, 240);
        painter.setPen(QPen(borderColor, 1));

        // Technologiczna ramka
        QPainterPath frame;
        int clipSize = 15;

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

        // Znaczniki AR w rogach
        painter.setPen(QPen(borderColor, 1, Qt::SolidLine));
        int markerSize = 8;

        // Lewy górny
        painter.drawLine(clipSize, 5, clipSize + markerSize, 5);
        painter.drawLine(clipSize, 5, clipSize, 5 + markerSize);

        // Prawy górny
        painter.drawLine(width() - clipSize - markerSize, 5, width() - clipSize, 5);
        painter.drawLine(width() - clipSize, 5, width() - clipSize, 5 + markerSize);

        // Prawy dolny
        painter.drawLine(width() - clipSize - markerSize, height() - 5, width() - clipSize, height() - 5);
        painter.drawLine(width() - clipSize, height() - 5, width() - clipSize, height() - 5 - markerSize);

        // Lewy dolny
        painter.drawLine(clipSize, height() - 5, clipSize + markerSize, height() - 5);
        painter.drawLine(clipSize, height() - 5, clipSize, height() - 5 - markerSize);

        // Dane techniczne w rogach
        painter.setPen(borderColor.lighter(120));
        painter.setFont(QFont("Consolas", 7));


        // // Linie skanowania (scanlines) - efekt monitora CRT
        // if (m_scanlineOpacity > 0.01) {
        //     painter.setPen(Qt::NoPen);
        //     painter.setBrush(QColor(0, 0, 0, 60 * m_scanlineOpacity));
        //
        //     for (int y = 0; y < height(); y += 3) {
        //         painter.drawRect(0, y, width(), 1);
        //     }
        // }
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        // Obsługa malowania spektrum audio
        if (watched == m_spectrumView && event->type() == QEvent::Paint) {
            paintSpectrum(m_spectrumView);
            return true;
        }
        return QFrame::eventFilter(watched, event);
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
        m_statusLabel->setText("WYSZUKIWANIE...");
    }

    void onSliderReleased() {
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

    void updateTimeLabel(int position) {
        if (!m_decoder || m_audioDuration <= 0)
            return;

        m_currentPosition = position / 1000.0; // Milisekundy na sekundy
        int currentSeconds = static_cast<int>(m_currentPosition);
        int totalSeconds = static_cast<int>(m_audioDuration);

        m_timeLabel->setText(QString("%1:%2 / %3:%4")
            .arg(currentSeconds / 60, 2, 10, QChar('0'))
            .arg(currentSeconds % 60, 2, 10, QChar('0'))
            .arg(totalSeconds / 60, 2, 10, QChar('0'))
            .arg(totalSeconds % 60, 2, 10, QChar('0')));
    }

    void updateSliderPosition(double position) {
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
        int seconds = int(position) % 60;
        int minutes = int(position) / 60;

        int totalSeconds = int(m_audioDuration) % 60;
        int totalMinutes = int(m_audioDuration) / 60;

        m_timeLabel->setText(
            QString("%1:%2 / %3:%4")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(totalMinutes, 2, 10, QChar('0'))
            .arg(totalSeconds, 2, 10, QChar('0'))
        );
    }

    void seekAudio(int position) {
        if (!m_decoder || m_audioDuration <= 0)
            return;

        double seekPos = position / 1000.0; // Milisekundy na sekundy
        m_decoder->seek(seekPos);
    }

    void togglePlayback() {
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

    void handleError(const QString& message) {
        qDebug() << "Audio decoder error:" << message;
        m_statusLabel->setText("ERROR: " + message.left(20));
        m_audioInfoLabel->setText("⚠️ " + message.left(30));
    }

    void handleAudioInfo(int sampleRate, int channels, double duration) {
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

    void updateUI() {
        // Aktualizacja stanu UI i animacje
        if (m_decoder && !m_decoder->isPaused() && !m_playbackFinished) {
            // Co pewien czas aktualizacja statusu
            int randomUpdate = QRandomGenerator::global()->bounded(100);
            if (randomUpdate < 2) {
                m_statusLabel->setText(QString("BUFFER: %1%").arg(QRandomGenerator::global()->bounded(95, 100)));
            } else if (randomUpdate < 4) {
                m_statusLabel->setText(QString("SYNC: %1%").arg(QRandomGenerator::global()->bounded(98, 100)));
            }

            // Aktualizacja danych spektrum - losowe fluktuacje dla efektów wizualnych
            for (int i = 0; i < m_spectrumData.size(); i++) {
                double intensity = m_spectrumIntensity;
                if (!m_decoder->isPaused()) {
                    // Bardziej dynamiczne zmiany podczas odtwarzania
                    double change = (QRandomGenerator::global()->bounded(100) - 50) / 250.0;
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

    void increaseSpectrumIntensity() {
        QPropertyAnimation* spectrumAnim = new QPropertyAnimation(this, "spectrumIntensity");
        spectrumAnim->setDuration(600);
        spectrumAnim->setStartValue(m_spectrumIntensity);
        spectrumAnim->setEndValue(0.6);
        spectrumAnim->setEasingCurve(QEasingCurve::OutCubic);
        spectrumAnim->start(QPropertyAnimation::DeleteWhenStopped);
    }

    void decreaseSpectrumIntensity() {
        QPropertyAnimation* spectrumAnim = new QPropertyAnimation(this, "spectrumIntensity");
        spectrumAnim->setDuration(800);
        spectrumAnim->setStartValue(m_spectrumIntensity);
        spectrumAnim->setEndValue(0.2);
        spectrumAnim->setEasingCurve(QEasingCurve::OutCubic);
        spectrumAnim->start(QPropertyAnimation::DeleteWhenStopped);
    }

private:
    void paintSpectrum(QWidget* target) {
        QPainter painter(target);
        painter.setRenderHint(QPainter::Antialiasing);

        // Wypełniamy tło
        painter.fillRect(target->rect(), QColor(10, 5, 20));

        // Rysujemy siatkę
        painter.setPen(QPen(QColor(50, 30, 90, 100), 1, Qt::DotLine));

        int stepX = target->width() / 8;
        for (int x = stepX; x < target->width(); x += stepX) {
            painter.drawLine(x, 0, x, target->height());
        }

        int stepY = target->height() / 3;
        for (int y = stepY; y < target->height(); y += stepY) {
            painter.drawLine(0, y, target->width(), y);
        }

        // Rysujemy słupki spektrum
        if (!m_spectrumData.isEmpty()) {
            int barCount = qMin(32, m_spectrumData.size());
            double barWidth = (double)target->width() / barCount;

            for (int i = 0; i < barCount; i++) {
                // Wysokość słupka zależna od danych i intensywności
                double height = m_spectrumData[i] * m_spectrumIntensity * target->height() * 0.9;

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

    // Pola klasy InlineAudioPlayer
    QLabel* m_audioInfoLabel;
    QLabel* m_statusLabel;
    QWidget* m_spectrumView;
    CyberAudioButton* m_playButton;
    CyberAudioSlider* m_progressSlider;
    QLabel* m_timeLabel;
    CyberAudioButton* m_volumeButton;
    CyberAudioSlider* m_volumeSlider;

    std::shared_ptr<AudioDecoder> m_decoder;
    QTimer* m_uiTimer;
    QTimer* m_spectrumTimer;

    QByteArray m_audioData;
    QString m_mimeType;

    double m_audioDuration = 0;
    double m_currentPosition = 0;
    bool m_sliderDragging = false;
    int m_lastVolume = 100;
    bool m_playbackFinished = false;
    bool m_wasPlaying = false;
    bool m_isActive = false;

    // Właściwości animacji
    double m_scanlineOpacity;
    double m_spectrumIntensity;
    QVector<double> m_spectrumData;
    
    static inline InlineAudioPlayer* s_activePlayer = nullptr;
};

#endif //INLINE_AUDIO_PLAYER_H