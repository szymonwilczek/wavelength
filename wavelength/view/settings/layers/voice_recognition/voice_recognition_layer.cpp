#include "voice_recognition_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QAudioFormat>
#include <QDebug>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <cmath>
#include <QPainterPath>

// Implementacja AudioVisualizer
AudioVisualizer::AudioVisualizer(QWidget *parent)
    : QWidget(parent), m_amplitude(0.0)
{
    setMinimumSize(300, 100);
    setMaximumHeight(100);
    
    // Inicjalizacja historii amplitud
    m_amplitudeHistory.resize(100, 0.0);
    
    // Timer do stopniowego zanikania wizualizacji
    m_fadeTimer = new QTimer(this);
    m_fadeTimer->setInterval(50);
    connect(m_fadeTimer, &QTimer::timeout, this, [this]() {
        for (int i = 0; i < m_amplitudeHistory.size(); ++i) {
            m_amplitudeHistory[i] *= 0.9;
        }
        update();
    });
    m_fadeTimer->start();
}

void AudioVisualizer::setAmplitude(qreal amplitude) {
    // Przesunięcie historii amplitud
    for (int i = 0; i < m_amplitudeHistory.size() - 1; ++i) {
        m_amplitudeHistory[i] = m_amplitudeHistory[i + 1];
    }
    
    // Dodanie nowej amplitudy
    m_amplitudeHistory[m_amplitudeHistory.size() - 1] = amplitude;
    
    update();
}

void AudioVisualizer::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int width = this->width();
    int height = this->height();
    
    // Tło
    painter.fillRect(rect(), QColor(10, 25, 40, 220));
    
    // Rysowanie siatki
    painter.setPen(QPen(QColor(50, 50, 50, 100), 1, Qt::DotLine));
    
    // Linie poziome
    for (int y = 20; y < height; y += 20) {
        painter.drawLine(0, y, width, y);
    }
    
    // Linie pionowe
    for (int x = 20; x < width; x += 20) {
        painter.drawLine(x, 0, x, height);
    }
    
    // Rysowanie fali dźwiękowej
    painter.setPen(QPen(QColor(255, 51, 51, 200), 2));
    
    QPainterPath path;
    bool pathStarted = false;
    
    int step = qMax(1, m_amplitudeHistory.size() / width);
    
    for (int i = 0; i < m_amplitudeHistory.size(); i += step) {
        qreal amplitude = m_amplitudeHistory[i];
        
        // Współrzędne X i Y dla punktu na fali
        qreal x = (i * width) / m_amplitudeHistory.size();
        qreal y = height / 2 - (amplitude * height / 2);
        
        if (!pathStarted) {
            path.moveTo(x, y);
            pathStarted = true;
        } else {
            path.lineTo(x, y);
        }
    }
    
    painter.drawPath(path);
    
    // Rysowanie fali lustrzanej (odbicie)
    painter.setPen(QPen(QColor(255, 51, 51, 100), 2));
    
    QPainterPath mirrorPath;
    pathStarted = false;
    
    for (int i = 0; i < m_amplitudeHistory.size(); i += step) {
        qreal amplitude = m_amplitudeHistory[i];
        
        qreal x = (i * width) / m_amplitudeHistory.size();
        qreal y = height / 2 + (amplitude * height / 2);
        
        if (!pathStarted) {
            mirrorPath.moveTo(x, y);
            pathStarted = true;
        } else {
            mirrorPath.lineTo(x, y);
        }
    }
    
    painter.drawPath(mirrorPath);
    
    // Linia środkowa
    painter.setPen(QPen(QColor(255, 51, 51, 150), 1));
    painter.drawLine(0, height / 2, width, height / 2);
    
    // Obramowanie
    painter.setPen(QPen(QColor(255, 51, 51), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

// Implementacja VoiceRecognitionLayer
VoiceRecognitionLayer::VoiceRecognitionLayer(QWidget *parent) 
    : SecurityLayer(parent),
      m_audioSource(nullptr),
      m_audioDevice(nullptr),
      m_currentAmplitude(0.0),
      m_lastAmplitude(0.0),
      m_silenceCount(0),
      m_voiceDetectedCount(0),
      m_isListening(false),
      m_voiceDetected(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("VOICE RECOGNITION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    m_instructionLabel = new QLabel("Please say: \"Wavelength system access requested\"", this);
    m_instructionLabel->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    m_instructionLabel->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel("Waiting for voice input...", this);
    m_statusLabel->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt; font-style: italic;");
    m_statusLabel->setAlignment(Qt::AlignCenter);

    // Visualizer
    m_visualizer = new AudioVisualizer(this);
    m_visualizer->setFixedHeight(100);
    m_visualizer->setStyleSheet("border: 1px solid #ff3333; border-radius: 5px;");

    m_detectionProgress = new QProgressBar(this);
    m_detectionProgress->setRange(0, 100);
    m_detectionProgress->setValue(0);
    m_detectionProgress->setTextVisible(false);
    m_detectionProgress->setFixedHeight(8);
    m_detectionProgress->setFixedWidth(300);
    m_detectionProgress->setStyleSheet(
        "QProgressBar {"
        "  background-color: rgba(30, 30, 30, 150);"
        "  border: 1px solid #333333;"
        "  border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #ff3333;"
        "  border-radius: 3px;"
        "}"
    );

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(m_instructionLabel);
    layout->addSpacing(10);
    layout->addWidget(m_visualizer);
    layout->addSpacing(10);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_detectionProgress);
    layout->addStretch();

    // Timer dla aktualizacji wizualizacji
    m_visualizerTimer = new QTimer(this);
    m_visualizerTimer->setInterval(50);
    connect(m_visualizerTimer, &QTimer::timeout, this, &VoiceRecognitionLayer::updateVisualization);
    
    // Timer do zakończenia rozpoznawania po wykryciu głosu
    m_detectionTimer = new QTimer(this);
    m_detectionTimer->setSingleShot(true);
    m_detectionTimer->setInterval(3000);
    connect(m_detectionTimer, &QTimer::timeout, this, &VoiceRecognitionLayer::finalizeVoiceRecognition);
}

VoiceRecognitionLayer::~VoiceRecognitionLayer() {
    stopListening();
    
    if (m_visualizerTimer) {
        m_visualizerTimer->stop();
        delete m_visualizerTimer;
        m_visualizerTimer = nullptr;
    }
    
    if (m_detectionTimer) {
        m_detectionTimer->stop();
        delete m_detectionTimer;
        m_detectionTimer = nullptr;
    }
}

void VoiceRecognitionLayer::initialize() {
    reset();
    setupAudio();
    QTimer::singleShot(500, this, &VoiceRecognitionLayer::startListening);
}

void VoiceRecognitionLayer::reset() {
    stopListening();
    
    m_currentAmplitude = 0.0;
    m_lastAmplitude = 0.0;
    m_silenceCount = 0;
    m_voiceDetectedCount = 0;
    m_isListening = false;
    m_voiceDetected = false;
    
    m_detectionProgress->setValue(0);
    m_statusLabel->setText("Waiting for voice input...");
    m_statusLabel->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt; font-style: italic;");
    
    if (m_visualizer) {
        m_visualizer->setAmplitude(0.0);
    }
}

void VoiceRecognitionLayer::setupAudio() {
    // Ustawienie formatu audio
    QAudioFormat format;
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    
    // Wybór urządzenia wejściowego domyślnego
    QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    
    if (!inputDevice.isFormatSupported(format)) {
        qWarning() << "Default format not supported, trying to use the nearest";
        format = inputDevice.preferredFormat();
    }
    
    // Tworzenie źródła audio
    m_audioSource = new QAudioSource(inputDevice, format, this);
    m_audioSource->setBufferSize(4096);
}

void VoiceRecognitionLayer::startListening() {
    if (m_isListening || !m_audioSource) {
        return;
    }
    
    m_isListening = true;
    m_statusLabel->setText("Listening for voice input...");
    
    // Otwarcie urządzenia do nasłuchiwania
    m_audioDevice = m_audioSource->start();
    
    if (m_audioDevice) {
        connect(m_audioDevice, &QIODevice::readyRead, this, &VoiceRecognitionLayer::processAudio);
    } else {
        qWarning() << "Could not open audio device!";
        m_statusLabel->setText("Error: Could not access microphone!");
        m_statusLabel->setStyleSheet("color: #ff5555; font-family: Consolas; font-size: 9pt; font-style: italic;");
    }
    
    // Uruchomienie timera wizualizacji
    m_visualizerTimer->start();
}

void VoiceRecognitionLayer::stopListening() {
    if (!m_isListening) {
        return;
    }
    
    m_isListening = false;
    
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    
    m_audioDevice = nullptr;
    
    if (m_visualizerTimer) {
        m_visualizerTimer->stop();
    }
}

void VoiceRecognitionLayer::processAudio() {
    if (!m_audioDevice || !m_isListening) {
        return;
    }
    
    // Odczytanie dostępnych danych
    m_audioBuffer = m_audioDevice->readAll();
    
    if (m_audioBuffer.isEmpty()) {
        return;
    }
    
    // Przetwarzanie próbek audio do obliczenia amplitudy
    qint16 *samples = reinterpret_cast<qint16*>(m_audioBuffer.data());
    int sampleCount = m_audioBuffer.size() / sizeof(qint16);
    
    // Obliczenie średniej amplitudy z bezwzględnych wartości próbek
    qreal sum = 0.0;
    for (int i = 0; i < sampleCount; ++i) {
        sum += qAbs(samples[i]);
    }
    
    // Normalizacja amplitudy do zakresu 0.0-1.0
    m_currentAmplitude = sum / (sampleCount * 32768.0);
    
    // Wykrywanie głosu
    if (m_currentAmplitude > AMPLITUDE_THRESHOLD) {
        m_voiceDetectedCount++;
        m_silenceCount = 0;
        
        // Jeśli wykryto wystarczająco dużo głosu, uznajemy to za wypowiedź
        if (m_voiceDetectedCount >= VOICE_DETECTION_FRAMES && !m_voiceDetected) {
            m_voiceDetected = true;
            m_statusLabel->setText("Voice detected! Processing...");
            m_statusLabel->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 9pt; font-style: italic;");
            
            // Uruchomienie timera do zakończenia rozpoznawania
            m_detectionTimer->start();
        }
    } else {
        m_silenceCount++;
        
        // Resetowanie licznika wykrytego głosu po dłuższej ciszy
        if (m_silenceCount > 5) {
            m_voiceDetectedCount = 0;
        }
    }
    
    // Aktualizacja paska postępu, jeśli głos jest wykrywany
    if (m_voiceDetected) {
        int progress = qMin(100, static_cast<int>((static_cast<float>(m_voiceDetectedCount) / REQUIRED_VOICE_DURATION) * 100));
        m_detectionProgress->setValue(progress);
    }
}

void VoiceRecognitionLayer::updateVisualization() {
    if (m_visualizer) {
        // Wygładzanie amplitudy
        qreal smoothedAmplitude = m_lastAmplitude * 0.7 + m_currentAmplitude * 0.3;
        m_lastAmplitude = smoothedAmplitude;
        
        // Aktualizacja wizualizera
        m_visualizer->setAmplitude(smoothedAmplitude);
    }
}

void VoiceRecognitionLayer::finalizeVoiceRecognition() {
    // Zatrzymanie nasłuchiwania
    stopListening();
    
    if (m_voiceDetected && m_voiceDetectedCount >= REQUIRED_VOICE_DURATION) {
        // Powodzenie - zmiana kolorów na zielone
        m_statusLabel->setText("Voice recognition successful!");
        m_statusLabel->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 9pt; font-weight: bold;");
        
        m_visualizer->setStyleSheet("border: 2px solid #33ff33; border-radius: 5px;");
        
        m_detectionProgress->setStyleSheet(
            "QProgressBar {"
            "  background-color: rgba(30, 30, 30, 150);"
            "  border: 1px solid #33ff33;"
            "  border-radius: 4px;"
            "}"
            "QProgressBar::chunk {"
            "  background-color: #33ff33;"
            "  border-radius: 3px;"
            "}"
        );
        
        // Małe opóźnienie przed animacją zanikania, aby pokazać zmianę kolorów
        QTimer::singleShot(1500, this, [this]() {
            QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(this);
            this->setGraphicsEffect(effect);

            QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
            animation->setDuration(500);
            animation->setStartValue(1.0);
            animation->setEndValue(0.0);
            animation->setEasingCurve(QEasingCurve::OutQuad);

            connect(animation, &QPropertyAnimation::finished, this, [this]() {
                emit layerCompleted();
            });

            animation->start(QPropertyAnimation::DeleteWhenStopped);
        });
    } else {
        // Niepowodzenie - reset i ponowna próba
        m_statusLabel->setText("Voice not recognized. Please try again.");
        m_statusLabel->setStyleSheet("color: #ff5555; font-family: Consolas; font-size: 9pt; font-style: italic;");
        
        QTimer::singleShot(2000, this, [this]() {
            reset();
            setupAudio();
            startListening();
        });
    }
}