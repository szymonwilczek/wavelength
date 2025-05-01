#ifndef COMMUNICATION_STREAM_H
#define COMMUNICATION_STREAM_H

#include <qcoreapplication.h>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include "stream_message.h"
#include "../labels/user_info_label.h"

struct UserVisuals {
    QColor color;
};

// Główny widget strumienia komunikacji - poprawiona wersja z OpenGL
class CommunicationStream final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    Q_PROPERTY(qreal waveAmplitude READ waveAmplitude WRITE setWaveAmplitude)
    Q_PROPERTY(qreal waveFrequency READ waveFrequency WRITE setWaveFrequency)
    Q_PROPERTY(qreal waveSpeed READ waveSpeed WRITE setWaveSpeed)
    Q_PROPERTY(qreal glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)
    Q_PROPERTY(qreal waveThickness READ waveThickness WRITE setWaveThickness)

public:
    enum StreamState {
        kIdle,        // Prosta linia z okazjonalnymi zakłóceniami
        kReceiving,   // Aktywna animacja podczas otrzymywania wiadomości
        kDisplaying   // Wyświetlanie wiadomości
    };

    explicit CommunicationStream(QWidget* parent = nullptr);

    ~CommunicationStream() override;

    StreamMessage* AddMessageWithAttachment(const QString& content, const QString& sender, StreamMessage::MessageType type, const QString& message_id = QString());

    // Settery i gettery dla animacji fali
    qreal GetWaveAmplitude() const { return wave_amplitude_; }
    void SetWaveAmplitude(qreal amplitude);

    qreal getWaveFrequency() const { return wave_frequency_; }
    void SetWaveFrequency(qreal frequency);

    qreal GetWaveSpeed() const { return wave_speed_; }
    void SetWaveSpeed(qreal speed);

    qreal GetGlitchIntensity() const { return glitch_intensity_; }
    void SetGlitchIntensity(qreal intensity);

    qreal GetWaveThickness() const { return wave_thickness_; }
    void SetWaveThickness(qreal thickness);

    void SetStreamName(const QString& name) const;

    StreamMessage* AddMessage(const QString &content, const QString &sender, StreamMessage::MessageType type, const QString& message_id = QString());

    void ClearMessages();

public slots:
    void SetTransmittingUser(const QString& user_id) const;

    // Slot do czyszczenia wskaźnika nadającego
    void ClearTransmittingUser() const;

    // Slot setAudioAmplitude (bez zmian)
    void SetAudioAmplitude(qreal amplitude);

protected:
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void UpdateAnimation();

    void TriggerRandomGlitch();

    void StartReceivingAnimation();

    void ReturnToIdleAnimation();

    void StartGlitchAnimation(qreal intensity);

    void ShowMessageAtIndex(int index);

    void ShowNextMessage();

    void ShowPreviousMessage();

    void OnMessageRead();

    void HandleMessageHidden();

private:
    static UserVisuals GenerateUserVisuals(const QString& user_id);

    void ConnectSignalsForMessage(StreamMessage* message);

    // disconnectSignalsForMessage: Rozłącza sygnały nawigacyjne, odczytu i ukrycia
    void DisconnectSignalsForMessage(StreamMessage* message) const;

    // Metoda pomocnicza do aktualizacji przycisków nawigacyjnych
    void UpdateNavigationButtonsForCurrentMessage();

    void OptimizeForMessageTransition() const;

    void UpdateMessagePosition();


    // Parametry animacji fali
    const qreal base_wave_amplitude_; // Bazowa amplituda
    const qreal amplitude_scale_;    // Skala dla amplitudy audio
    qreal wave_amplitude_;           // Aktualna amplituda (animowana)
    qreal target_wave_amplitude_;
    UserInfoLabel* transmitting_user_label_;
    qreal wave_frequency_;
    qreal wave_speed_;
    qreal glitch_intensity_;
    qreal wave_thickness_;

    StreamState state_;
    QList<StreamMessage*> messages_;  // Zmienione z QQueue na QList dla indeksowania
    int current_message_index_;
    bool is_clearing_all_messages_ = false;

    QLabel* stream_name_label_;
    bool initialized_;
    qreal time_offset_;

    QTimer* animation_timer_;
    QTimer* glitch_timer_;

    // OpenGL
    QOpenGLShaderProgram* shader_program_;
    QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vertex_buffer_;
};

#endif // COMMUNICATION_STREAM_H