#ifndef VOICE_RECOGNITION_LAYER_H
#define VOICE_RECOGNITION_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QAudioInput>
#include <QAudioFormat>
#include <QIODevice>
#include <QByteArray>
#include <QWidget>

// Widżet do wizualizacji fali dźwiękowej
class AudioVisualizer : public QWidget {
    Q_OBJECT
public:
    explicit AudioVisualizer(QWidget *parent = nullptr);
    void setAmplitude(qreal amplitude);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal m_amplitude;
    QVector<qreal> m_amplitudeHistory;
    QTimer* m_fadeTimer;
};

class VoiceRecognitionLayer : public SecurityLayer {
    Q_OBJECT

public:
    explicit VoiceRecognitionLayer(QWidget *parent = nullptr);
    ~VoiceRecognitionLayer() override;

    void initialize() override;
    void reset() override;

    private slots:
        void processAudio();
    void updateVisualization();
    void finalizeVoiceRecognition();

private:
    void setupAudio();
    void startListening();
    void stopListening();

    QLabel* m_instructionLabel;
    QLabel* m_statusLabel;
    QProgressBar* m_detectionProgress;
    AudioVisualizer* m_visualizer;

    QAudioInput* m_audioSource;
    QIODevice* m_audioDevice;
    QByteArray m_audioBuffer;

    QTimer* m_visualizerTimer;
    QTimer* m_detectionTimer;

    qreal m_currentAmplitude;
    qreal m_lastAmplitude;
    int m_silenceCount;
    int m_voiceDetectedCount;
    bool m_isListening;
    bool m_voiceDetected;

    // Progi dla wykrywania głosu
    const qreal AMPLITUDE_THRESHOLD = 0.05;
    const int VOICE_DETECTION_FRAMES = 10;  // Liczba ramek z głosem powyżej progu
    const int REQUIRED_VOICE_DURATION = 20; // Wymagana liczba ramek z głosem do uznania za wypowiedź
};

#endif // VOICE_RECOGNITION_LAYER_H