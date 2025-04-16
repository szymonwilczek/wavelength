#ifndef FLOATING_ENERGY_SPHERE_WIDGET_H
#define FLOATING_ENERGY_SPHERE_WIDGET_H

#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPoint>
#include <vector>
#include <QtMultimedia/QMediaPlayer> // <<< Dodano
#include <QtMultimedia/QAudioDecoder> // <<< Dodano
#include <QtMultimedia/QAudioBuffer> // <<< Dodano
#include <QMediaContent> // <<< Dodano
#include <QAudioFormat> // <<< Dodano
#include <QVector>

#define MAX_IMPACTS 5

struct ImpactInfo {
    QVector3D point = QVector3D(0,0,0); // Punkt uderzenia w lokalnych koordynatach sfery (znormalizowany)
    float startTime = -1.0f;       // Czas rozpoczęcia uderzenia (m_timeValue)
    bool active = false;           // Czy ten slot jest aktywny
};

class FloatingEnergySphereWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit FloatingEnergySphereWidget(bool isFirstTime, QWidget *parent = nullptr);
    ~FloatingEnergySphereWidget() override;

    void setClosable(bool closable);

    signals:
        void widgetClosed();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    private slots:
        void updateAnimation();
        void processAudioBuffer(); // <<< Dodano
        void audioDecodingFinished(); // <<< Dodano
        void handleMediaPlayerError(); // <<< Dodano
        void handleAudioDecoderError(); // <<< Dodano


private:
    void setupSphereGeometry(int rings, int sectors);
    void setupShaders();
    void setupAudio(); // <<< Dodano
    float calculateRMSAmplitude(const QAudioBuffer& buffer); // <<< Dodano

    bool m_isFirstTime;

    QTimer m_timer;
    float m_timeValue;

    QOpenGLShaderProgram *m_program;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;

    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_modelMatrix;

    QVector3D m_cameraPosition;
    float m_cameraDistance;

    QPoint m_lastMousePos;
    bool m_mousePressed;
    QQuaternion m_rotation;

    std::vector<GLfloat> m_vertices;
    int m_vertexCount;

    bool m_closable;

    QVector3D m_angularVelocity;
    float m_dampingFactor;

    QElapsedTimer m_elapsedTimer;
    float m_lastFrameTimeSecs;

    // --- Audio ---
    QMediaPlayer *m_player; // <<< Dodano
    QAudioDecoder *m_decoder; // <<< Dodano
    std::vector<float> m_audioAmplitudes; // <<< Dodano: Przechowuje znormalizowane amplitudy RMS dla każdego bufora
    QAudioFormat m_audioFormat; // <<< Dodano: Przechowuje format dekodowanego audio
    qint64 m_audioBufferDurationMs; // <<< Dodano: Czas trwania jednego bufora audio w ms
    float m_currentAudioAmplitude; // <<< Dodano: Aktualna amplituda do przekazania do shadera
    float m_targetAudioAmplitude;
    bool m_audioReady; // <<< Dodano: Flaga wskazująca, czy dane audio są gotowe

    std::vector<ImpactInfo> m_impacts;
    int m_nextImpactIndex;
};

#endif // FLOATING_ENERGY_SPHERE_WIDGET_H