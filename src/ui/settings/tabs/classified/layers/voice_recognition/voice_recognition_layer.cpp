#include "voice_recognition_layer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPainterPath>

VoiceRecognitionLayer::VoiceRecognitionLayer(QWidget *parent) 
    : SecurityLayer(parent),
      m_progressTimer(nullptr),
      m_recognitionTimer(nullptr),
      m_audioProcessTimer(nullptr),
      m_audioInput(nullptr),
      m_audioDevice(nullptr),
      m_isRecording(false),
      m_noiseThreshold(0.05f),  // Próg detekcji mowy - można dostosować eksperymentalnie
      m_isSpeaking(false),
      m_silenceCounter(0),
      m_currentAudioLevel(0.0f)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("VOICE RECOGNITION VERIFICATION", this);
    title->setStyleSheet("color: #ff3333; font-family: Consolas; font-size: 11pt;");
    title->setAlignment(Qt::AlignCenter);

    // Wizualizator audio - widget do rysowania
    m_audioVisualizerLabel = new QLabel(this);
    m_audioVisualizerLabel->setFixedSize(400, 200);
    m_audioVisualizerLabel->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 5px;");
    m_audioVisualizerLabel->setAlignment(Qt::AlignCenter);

    // Inicjalizacja danych wizualizatora
    m_visualizerData.resize(100);
    m_visualizerData.fill(0);

    QLabel *instructions = new QLabel("Proszę mówić do mikrofonu\nWeryfikacja wzorca głosowego w toku", this);
    instructions->setStyleSheet("color: #aaaaaa; font-family: Consolas; font-size: 9pt;");
    instructions->setAlignment(Qt::AlignCenter);

    m_recognitionProgress = new QProgressBar(this);
    m_recognitionProgress->setRange(0, 100);
    m_recognitionProgress->setValue(0);
    m_recognitionProgress->setTextVisible(false);
    m_recognitionProgress->setFixedHeight(8);
    m_recognitionProgress->setFixedWidth(400);
    m_recognitionProgress->setStyleSheet(
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
    layout->addWidget(m_audioVisualizerLabel, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(instructions);
    layout->addWidget(m_recognitionProgress, 0, Qt::AlignCenter);
    layout->addStretch();

    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(100);
    connect(m_progressTimer, &QTimer::timeout, this, &VoiceRecognitionLayer::updateProgress);

    m_recognitionTimer = new QTimer(this);
    m_recognitionTimer->setSingleShot(true);
    m_recognitionTimer->setInterval(8000); // 8 sekund nagrywania
    connect(m_recognitionTimer, &QTimer::timeout, this, &VoiceRecognitionLayer::finishRecognition);

    m_audioProcessTimer = new QTimer(this);
    m_audioProcessTimer->setInterval(50); // Odświeżanie co 50ms
    connect(m_audioProcessTimer, &QTimer::timeout, this, &VoiceRecognitionLayer::processAudioInput);
}

VoiceRecognitionLayer::~VoiceRecognitionLayer() {
    stopRecording();

    if (m_progressTimer) {
        m_progressTimer->stop();
        delete m_progressTimer;
        m_progressTimer = nullptr;
    }

    if (m_recognitionTimer) {
        m_recognitionTimer->stop();
        delete m_recognitionTimer;
        m_recognitionTimer = nullptr;
    }

    if (m_audioProcessTimer) {
        m_audioProcessTimer->stop();
        delete m_audioProcessTimer;
        m_audioProcessTimer = nullptr;
    }
}

void VoiceRecognitionLayer::initialize() {
    reset(); // Reset najpierw

    // Upewnij się, że widget jest widoczny przed startem
    if (graphicsEffect()) {
         static_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(1.0);
    }

    // Rozpocznij nagrywanie po krótkim opóźnieniu
    QTimer::singleShot(500, this, &VoiceRecognitionLayer::startRecording); // Krótsze opóźnienie
}

void VoiceRecognitionLayer::reset() {
    stopRecording(); // Zatrzymaj nagrywanie i przetwarzanie audio

    // Zatrzymaj pozostałe timery
    if (m_progressTimer && m_progressTimer->isActive()) {
        m_progressTimer->stop();
    }
    if (m_recognitionTimer && m_recognitionTimer->isActive()) {
        m_recognitionTimer->stop();
    }

    // Resetuj stan
    m_recognitionProgress->setValue(0);
    m_audioBuffer.clear();
    m_visualizerData.fill(0);
    m_isSpeaking = false;
    m_silenceCounter = 0;
    m_currentAudioLevel = 0.0f;

    // Przywróć domyślne style
    m_audioVisualizerLabel->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 1px solid #ff3333; border-radius: 5px;"); // Czerwony border
    m_recognitionProgress->setStyleSheet(
        "QProgressBar {"
        "  background-color: rgba(30, 30, 30, 150);"
        "  border: 1px solid #333333;" // Szary border
        "  border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #ff3333;" // Czerwony chunk
        "  border-radius: 3px;"
        "}"
    );

    // Wyczyść wizualizator
    updateAudioVisualizer(QByteArray());

    // --- KLUCZOWA ZMIANA: Przywróć przezroczystość ---
    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect());
    if (effect) {
        effect->setOpacity(1.0);
        // Opcjonalnie: Można usunąć efekt
        // this->setGraphicsEffect(nullptr);
        // delete effect;
    }
}

void VoiceRecognitionLayer::startRecording() {
    if (m_isRecording)
        return;

    // Ustawienie formatu audio
    QAudioFormat format;
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");

    // Sprawdzenie dostępnych urządzeń
    QAudioDeviceInfo inputDevice = QAudioDeviceInfo::defaultInputDevice();
    if (!inputDevice.isFormatSupported(format)) {
        format = inputDevice.nearestFormat(format);
    }

    // Inicjalizacja źródła audio
    m_audioInput = new QAudioInput(inputDevice, format, this);
    m_audioDevice = m_audioInput->start();

    if (m_audioDevice) {
        m_isRecording = true;
        m_progressTimer->start();
        m_recognitionTimer->start();
        m_audioProcessTimer->start();
    }
}

void VoiceRecognitionLayer::stopRecording() {
    if (!m_isRecording)
        return;

    if (m_audioInput) {
        m_audioInput->stop();
        delete m_audioInput;
        m_audioInput = nullptr;
        m_audioDevice = nullptr;
    }

    if (m_audioProcessTimer && m_audioProcessTimer->isActive()) {
        m_audioProcessTimer->stop();
    }

    m_isRecording = false;
}

bool VoiceRecognitionLayer::isSpeaking(float audioLevel) {
    return audioLevel > m_noiseThreshold;
}

void VoiceRecognitionLayer::processAudioInput() {
    if (!m_audioDevice || !m_isRecording)
        return;

    // Odczytaj dane audio
    QByteArray data;
    qint64 len = m_audioInput->bytesReady();

    if (len > 0) {
        data.resize(len);
        m_audioDevice->read(data.data(), len);

        // Obliczenie poziomu głośności dla progu detekcji mowy
        float currentLevel = 0.0f;
        if (data.size() > 0) {
            const qint16 *samples = reinterpret_cast<const qint16*>(data.constData());
            int numSamples = data.size() / sizeof(qint16);

            // Obliczenie poziomu dźwięku jako średniej wartości bezwzględnej
            float sum = 0;
            for (int i = 0; i < numSamples; i++) {
                sum += qAbs(samples[i]) / 32768.0f; // Normalizacja do 0-1
            }

            if (numSamples > 0) {
                currentLevel = sum / numSamples;
            }
        }

        // Aktualizacja stanu mówienia z zastosowaniem histerezy
        // Histereza zapobiega szybkiemu przełączaniu między stanami przy granicznych wartościach
        if (m_isSpeaking) {
            // Jeśli obecnie wykrywamy mowę, stosujemy niższy próg dla wyłączenia
            m_isSpeaking = currentLevel > (m_noiseThreshold * 0.8f);
        } else {
            // Jeśli obecnie nie wykrywamy mowy, stosujemy wyższy próg dla włączenia
            m_isSpeaking = currentLevel > (m_noiseThreshold * 1.2f);
        }

        // Aktualizuj licznik ciszy
        if (m_isSpeaking) {
            m_silenceCounter = 0;

            // Dodaj dane do bufora tylko gdy użytkownik mówi
            m_audioBuffer.append(data);
        } else {
            m_silenceCounter += m_audioProcessTimer->interval();
        }

        m_currentAudioLevel = currentLevel;
        updateAudioVisualizer(data);
    }
}

void VoiceRecognitionLayer::updateAudioVisualizer(const QByteArray &data) {
    // Konwersja danych audio na wartości wizualizatora
    if (data.size() > 0) {
        const qint16 *samples = reinterpret_cast<const qint16*>(data.constData());
        int numSamples = data.size() / sizeof(qint16);

        // Przesunięcie wszystkich wartości w lewo
        for (int i = 0; i < m_visualizerData.size() - 1; i++) {
            m_visualizerData[i] = m_visualizerData[i + 1];
        }

        // Dodanie nowej wartości na koniec (średnia z próbek)
        float sum = 0;
        for (int i = 0; i < numSamples; i++) {
            sum += qAbs(samples[i]) / 32768.0f; // Normalizacja do 0-1
        }

        if (numSamples > 0) {
            m_visualizerData[m_visualizerData.size() - 1] = sum / numSamples;
        }
    }

    // Rysowanie wizualizacji
    QImage visualizer(400, 200, QImage::Format_ARGB32);
    visualizer.fill(Qt::transparent);

    QPainter painter(&visualizer);
    painter.setRenderHint(QPainter::Antialiasing);

    // Rysowanie tła
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(10, 25, 40, 220));
    painter.drawRect(0, 0, visualizer.width(), visualizer.height());

    // Rysowanie linii progu detekcji mowy
    int thresholdY = visualizer.height() / 2 - m_noiseThreshold * 80.0f;
    int thresholdY2 = visualizer.height() / 2 + m_noiseThreshold * 80.0f;
    painter.setPen(QPen(QColor(200, 200, 200, 100), 1, Qt::DashLine));
    painter.drawLine(0, thresholdY, visualizer.width(), thresholdY);
    painter.drawLine(0, thresholdY2, visualizer.width(), thresholdY2);

    // Rysowanie wizualizacji dźwięku
    QPainterPath path;
    int centerY = visualizer.height() / 2;
    int barWidth = visualizer.width() / m_visualizerData.size();

    // Wybór koloru w zależności od tego, czy wykryto mowę
    QColor waveColor = m_isSpeaking ? QColor(51, 153, 255, 200) : QColor(180, 180, 180, 150);
    QColor fillColor = m_isSpeaking ? QColor(51, 153, 255, 50) : QColor(100, 100, 100, 30);

    painter.setPen(QPen(waveColor, 2));
    painter.setBrush(QBrush(fillColor));

    // Górna część fali
    path.moveTo(0, centerY);
    for (int i = 0; i < m_visualizerData.size(); i++) {
        float amplitude = m_visualizerData[i] * 80.0f; // Skalowanie amplitudy
        path.lineTo(i * barWidth, centerY - amplitude);
    }

    // Dolna część fali (lustrzane odbicie)
    for (int i = m_visualizerData.size() - 1; i >= 0; i--) {
        float amplitude = m_visualizerData[i] * 80.0f;
        path.lineTo(i * barWidth, centerY + amplitude);
    }

    path.closeSubpath();
    painter.drawPath(path);

    // Dodanie siatki
    painter.setPen(QPen(QColor(100, 100, 100, 50), 1, Qt::DotLine));
    for (int i = 0; i < 4; i++) {
        int y = visualizer.height() * (i + 1) / 5;
        painter.drawLine(0, y, visualizer.width(), y);
    }

    for (int i = 0; i < 5; i++) {
        int x = visualizer.width() * i / 5;
        painter.drawLine(x, 0, x, visualizer.height());
    }

    // Dodanie wskaźnika stanu mowy
    QString speakingStatus = m_isSpeaking ? "Mowa wykryta" : "Cisza";
    painter.setPen(QPen(m_isSpeaking ? Qt::green : Qt::red));
    painter.drawText(10, 20, speakingStatus);

    painter.end();

    m_audioVisualizerLabel->setPixmap(QPixmap::fromImage(visualizer));
}

void VoiceRecognitionLayer::updateProgress() {
    int currentValue = m_recognitionProgress->value();
    int newValue = currentValue;

    // Aktualizacja postępu w zależności od stanu mowy
    if (m_isSpeaking) {
        // Zwiększ postęp - ale wolniej
        newValue = currentValue + 2; // Zmniejszono przyrost dla dłuższego testu
        if (newValue > 100) newValue = 100;
        m_recognitionProgress->setValue(newValue);
        // qDebug() << "Progress updated to:" << newValue; // Logowanie postępu

        // Zakończ weryfikację TYLKO gdy postęp osiągnie 100%
        if (newValue >= 100) {
            // Zatrzymaj timer postępu, aby uniknąć wielokrotnego wywołania finishRecognition
            if (m_progressTimer->isActive()) {
                m_progressTimer->stop();
            }
            finishRecognition();
        }
    }
}

void VoiceRecognitionLayer::finishRecognition() {
    stopRecording();
    m_progressTimer->stop();

    // Zmiana kolorów na zielony po pomyślnym skanowaniu
    m_audioVisualizerLabel->setStyleSheet("background-color: rgba(10, 25, 40, 220); border: 2px solid #33ff33; border-radius: 5px;");

    m_recognitionProgress->setStyleSheet(
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
    m_recognitionProgress->setValue(100);

    // Renderowanie finalnej wizualizacji w zielonym kolorze
    QImage finalVisualizer(400, 200, QImage::Format_ARGB32);
    finalVisualizer.fill(Qt::transparent);

    QPainter painter(&finalVisualizer);
    painter.setRenderHint(QPainter::Antialiasing);

    // Rysowanie tła
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(10, 25, 40, 220));
    painter.drawRect(0, 0, finalVisualizer.width(), finalVisualizer.height());

    // Rysowanie wizualizacji dźwięku w kolorze zielonym
    QPainterPath path;
    int centerY = finalVisualizer.height() / 2;
    int barWidth = finalVisualizer.width() / m_visualizerData.size();

    painter.setPen(QPen(QColor(50, 200, 50, 200), 2));
    painter.setBrush(QBrush(QColor(50, 200, 50, 50)));

    // Górna część fali
    path.moveTo(0, centerY);
    for (int i = 0; i < m_visualizerData.size(); i++) {
        float amplitude = m_visualizerData[i] * 80.0f;
        path.lineTo(i * barWidth, centerY - amplitude);
    }

    // Dolna część fali
    for (int i = m_visualizerData.size() - 1; i >= 0; i--) {
        float amplitude = m_visualizerData[i] * 80.0f;
        path.lineTo(i * barWidth, centerY + amplitude);
    }

    path.closeSubpath();
    painter.drawPath(path);

    // Tekst sukcesu
    painter.setPen(QPen(Qt::green));
    painter.setFont(QFont("Consolas", 12));
    painter.drawText(finalVisualizer.rect(), Qt::AlignCenter, "Weryfikacja głosu zakończona");

    painter.end();

    m_audioVisualizerLabel->setPixmap(QPixmap::fromImage(finalVisualizer));
    
    // Animacja zanikania po krótkim pokazaniu sukcesu
    QTimer::singleShot(800, this, [this]() {
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
}