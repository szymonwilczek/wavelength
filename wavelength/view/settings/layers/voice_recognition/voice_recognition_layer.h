#ifndef VOICE_RECOGNITION_LAYER_H
#define VOICE_RECOGNITION_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QAudioInput>
#include <QAudioDeviceInfo>
#include <QIODevice>
#include <QByteArray>
#include <QAudioFormat>
#include <QBuffer>

class VoiceRecognitionLayer : public SecurityLayer {
    Q_OBJECT

public:
    explicit VoiceRecognitionLayer(QWidget *parent = nullptr);
    ~VoiceRecognitionLayer() override;

    void initialize() override;
    void reset() override;

    private slots:
        void processAudioInput();
    void updateProgress();
    void finishRecognition();

private:
    void startRecording();
    void stopRecording();
    void updateAudioVisualizer(const QByteArray &data);
    bool isSpeaking(float audioLevel);

    QLabel* m_audioVisualizerLabel;
    QProgressBar* m_recognitionProgress;
    QTimer* m_progressTimer;
    QTimer* m_recognitionTimer;
    QTimer* m_audioProcessTimer;

    QAudioInput* m_audioInput;
    QIODevice* m_audioDevice;

    QByteArray m_audioBuffer;
    QVector<float> m_visualizerData;
    bool m_isRecording;

    // Zmienne do wykrywania mowy i zarządzania postępem
    float m_noiseThreshold;   // Próg powyżej którego uznajemy, że użytkownik mówi
    bool m_isSpeaking;        // Czy użytkownik aktualnie mówi
    int m_silenceCounter;     // Licznik ciszy (w milisekundach)
    float m_currentAudioLevel; // Bieżący poziom dźwięku
};

#endif // VOICE_RECOGNITION_LAYER_H