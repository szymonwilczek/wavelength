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
#include <memory>
#include <QApplication>

#include "../../audio/decoder/audio_decoder.h"

// Klasa odtwarzacza audio zintegrowanego z czatem
class InlineAudioPlayer : public QFrame {
    Q_OBJECT

public:

    static InlineAudioPlayer* getActivePlayer() {
        return s_activePlayer;
    }

    InlineAudioPlayer(const QByteArray& audioData, const QString& mimeType, QWidget* parent = nullptr)
        : QFrame(parent), m_audioData(audioData), m_mimeType(mimeType) {

        setFrameStyle(QFrame::StyledPanel);
        setStyleSheet("background-color: #1a1a1a; border: 1px solid #444;");
        // Mniejszy rozmiar dla odtwarzacza audio
        setFixedSize(480, 120);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(10);

        // Obszar tytułu/informacji o pliku audio
        m_audioInfoLabel = new QLabel(this);
        m_audioInfoLabel->setAlignment(Qt::AlignCenter);
        m_audioInfoLabel->setStyleSheet("color: #ffffff; font-size: 12pt;");
        m_audioInfoLabel->setText("Odtwarzacz audio");
        layout->addWidget(m_audioInfoLabel);

        // Ikona audio / wizualizacja
        m_audioIcon = new QLabel(this);
        m_audioIcon->setAlignment(Qt::AlignCenter);
        m_audioIcon->setStyleSheet("color: #85c4ff; font-size: 24pt;");
        m_audioIcon->setText("🎵");  // Ikona muzyki
        layout->addWidget(m_audioIcon);

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
            m_playButton->setText("↻"); // Symbol powtórzenia zamiast pauzy
        });

        // Dodajemy obsługę kontroli dźwięku
        connect(m_volumeSlider, &QSlider::valueChanged, this, &InlineAudioPlayer::adjustVolume);
        connect(m_volumeButton, &QToolButton::clicked, this, &InlineAudioPlayer::toggleMute);

        // Nowe połączenie dla aktualizacji pozycji
        connect(m_decoder.get(), &AudioDecoder::positionChanged, this, &InlineAudioPlayer::updateSliderPosition);

        // Inicjalizuj odtwarzacz w osobnym wątku
        QTimer::singleShot(100, this, [this]() {
            m_decoder->start(QThread::HighPriority);
        });

        // Timer do aktualizacji pozycji suwaka
        m_updateTimer = new QTimer(this);
        m_updateTimer->setInterval(100);
        connect(m_updateTimer, &QTimer::timeout, this, &InlineAudioPlayer::updateProgressUI);
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

    ~InlineAudioPlayer() {
        releaseResources();
    }

    void releaseResources() {
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
            m_volumeSlider->setValue(m_lastVolume);
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
        seekAudio(m_progressSlider->value());

        m_sliderDragging = false;

        // Przywróć odtwarzanie jeśli było aktywne wcześniej
        if (m_wasPlaying) {
            m_decoder->pause(); // Przełącza stan pauzy (wznawia)
            m_playButton->setText("⏸");
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
            m_playButton->setText("❚❚"); // Symbol pauzy
            m_currentPosition = 0;
            updateTimeLabel(0);
            m_decoder->pause(); // Rozpocznij odtwarzanie (przełącz stan)
        } else {
            if (m_decoder->isPaused()) {
                m_playButton->setText("❚❚"); // Symbol pauzy
                m_decoder->pause(); // Wznów odtwarzanie
            } else {
                m_playButton->setText("▶"); // Symbol odtwarzania
                m_decoder->pause(); // Wstrzymaj odtwarzanie
            }
        }
    }

    void handleError(const QString& message) {
        qDebug() << "Audio decoder error:" << message;
        m_audioInfoLabel->setText("⚠️ " + message);
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
    }

private:
    QLabel* m_audioInfoLabel;
    QLabel* m_audioIcon;
    QPushButton* m_playButton;
    QSlider* m_progressSlider;
    QLabel* m_timeLabel;
    QToolButton* m_volumeButton;
    QSlider* m_volumeSlider;
    std::shared_ptr<AudioDecoder> m_decoder;
    QTimer* m_updateTimer;

    QByteArray m_audioData;
    QString m_mimeType;

    double m_audioDuration = 0;
    double m_currentPosition = 0;
    bool m_sliderDragging = false;
    int m_lastVolume = 100; // Domyślny poziom głośności
    bool m_playbackFinished = false;
    bool m_wasPlaying = false;
    bool m_isActive = false;  // Czy ten odtwarzacz jest aktywny
    
    static inline InlineAudioPlayer* s_activePlayer = nullptr;
};

#endif //INLINE_AUDIO_PLAYER_H