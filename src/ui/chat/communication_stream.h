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
class CommunicationStream : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    Q_PROPERTY(qreal waveAmplitude READ waveAmplitude WRITE setWaveAmplitude)
    Q_PROPERTY(qreal waveFrequency READ waveFrequency WRITE setWaveFrequency)
    Q_PROPERTY(qreal waveSpeed READ waveSpeed WRITE setWaveSpeed)
    Q_PROPERTY(qreal glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)
    Q_PROPERTY(qreal waveThickness READ waveThickness WRITE setWaveThickness)

public:
    enum StreamState {
        Idle,        // Prosta linia z okazjonalnymi zakłóceniami
        Receiving,   // Aktywna animacja podczas otrzymywania wiadomości
        Displaying   // Wyświetlanie wiadomości
    };

    explicit CommunicationStream(QWidget* parent = nullptr);

    ~CommunicationStream() override;

    StreamMessage* addMessageWithAttachment(const QString& content, const QString& sender, StreamMessage::MessageType type, const QString& messageId = QString());

    // Settery i gettery dla animacji fali
    qreal waveAmplitude() const { return m_waveAmplitude; }
    void setWaveAmplitude(qreal amplitude);

    qreal waveFrequency() const { return m_waveFrequency; }
    void setWaveFrequency(qreal frequency);

    qreal waveSpeed() const { return m_waveSpeed; }
    void setWaveSpeed(qreal speed);

    qreal glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(qreal intensity);

    qreal waveThickness() const { return m_waveThickness; }
    void setWaveThickness(qreal thickness);

    void setStreamName(const QString& name);

    StreamMessage* addMessage(const QString &content, const QString &sender, StreamMessage::MessageType type, const QString& messageId = QString());

    void clearMessages();

public slots:
    void setTransmittingUser(const QString& userId);

    // Slot do czyszczenia wskaźnika nadającego
    void clearTransmittingUser();

    // Slot setAudioAmplitude (bez zmian)
    void setAudioAmplitude(qreal amplitude);

protected:
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void updateAnimation();


    void triggerRandomGlitch();

    void startReceivingAnimation();

    void returnToIdleAnimation();

    void startGlitchAnimation(qreal intensity);

    void showMessageAtIndex(int index);

    void showNextMessage();

    void showPreviousMessage();

    void onMessageRead();

    void handleMessageHidden();

private:
    static UserVisuals generateUserVisuals(const QString& userId);

    void connectSignalsForMessage(StreamMessage* message);

    // disconnectSignalsForMessage: Rozłącza sygnały nawigacyjne, odczytu i ukrycia
    void disconnectSignalsForMessage(StreamMessage* message) const;

    // Metoda pomocnicza do aktualizacji przycisków nawigacyjnych
    void updateNavigationButtonsForCurrentMessage();

    void optimizeForMessageTransition() const;

    void updateMessagePosition();


    // Parametry animacji fali
    const qreal m_baseWaveAmplitude; // Bazowa amplituda
    const qreal m_amplitudeScale;    // Skala dla amplitudy audio
    qreal m_waveAmplitude;           // Aktualna amplituda (animowana)
    qreal m_targetWaveAmplitude;
    UserInfoLabel* m_transmittingUserLabel;
    qreal m_waveFrequency;
    qreal m_waveSpeed;
    qreal m_glitchIntensity;
    qreal m_waveThickness;

    StreamState m_state;
    QList<StreamMessage*> m_messages;  // Zmienione z QQueue na QList dla indeksowania
    int m_currentMessageIndex;
    bool m_isClearingAllMessages = false;

    QLabel* m_streamNameLabel;
    bool m_initialized;
    qreal m_timeOffset;

    QTimer* m_animTimer;
    QTimer* m_glitchTimer;

    // OpenGL
    QOpenGLShaderProgram* m_shaderProgram;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vertexBuffer;
};

#endif // COMMUNICATION_STREAM_H