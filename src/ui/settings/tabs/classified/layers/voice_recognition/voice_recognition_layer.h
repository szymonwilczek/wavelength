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

class VoiceRecognitionLayer final : public SecurityLayer {
    Q_OBJECT

public:
    explicit VoiceRecognitionLayer(QWidget *parent = nullptr);
    ~VoiceRecognitionLayer() override;

    void Initialize() override;
    void Reset() override;

    private slots:
        void ProcessAudioInput();
        void UpdateProgress();
        void FinishRecognition();

private:
    void StartRecording();
    void StopRecording();
    void UpdateAudioVisualizer(const QByteArray &data);
    bool IsSpeaking(float audio_level) const;

    QLabel* audio_visualizer_label_;
    QProgressBar* recognition_progress_;
    QTimer* progress_timer_;
    QTimer* recognition_timer_;
    QTimer* audio_process_timer_;

    QAudioInput* audio_input_;
    QIODevice* audio_device_;

    QByteArray audio_buffer_;
    QVector<float> visualizer_data_;
    bool is_recording_;

    // Zmienne do wykrywania mowy i zarządzania postępem
    float noise_threshold_;   // Próg powyżej którego uznajemy, że użytkownik mówi
    bool is_speaking_;        // Czy użytkownik aktualnie mówi
    int silence_counter_;     // Licznik ciszy (w milisekundach)
    float current_audio_level_; // Bieżący poziom dźwięku
};

#endif // VOICE_RECOGNITION_LAYER_H