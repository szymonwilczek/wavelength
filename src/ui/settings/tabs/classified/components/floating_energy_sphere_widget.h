#ifndef FLOATING_ENERGY_SPHERE_WIDGET_H
#define FLOATING_ENERGY_SPHERE_WIDGET_H

#include <deque>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QTimer>
#include <QWheelEvent>
#include <QPoint>
#include <vector>
#include <QtMultimedia/QMediaPlayer> // <<< Dodano
#include <QtMultimedia/QAudioDecoder> // <<< Dodano
#include <QtMultimedia/QAudioBuffer> // <<< Dodano
#include <QMediaContent> // <<< Dodano
#include <QAudioFormat> // <<< Dodano

#define MAX_IMPACTS 5

struct ImpactInfo {
    QVector3D point = QVector3D(0,0,0); // Punkt uderzenia w lokalnych koordynatach sfery (znormalizowany)
    float start_time = -1.0f;       // Czas rozpoczęcia uderzenia (m_timeValue)
    bool active = false;           // Czy ten slot jest aktywny
};

class FloatingEnergySphereWidget final : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit FloatingEnergySphereWidget(bool is_first_time, QWidget *parent = nullptr);
    ~FloatingEnergySphereWidget() override;

    void SetClosable(bool closable);

    signals:
        void widgetClosed();
        void konamiCodeEntered();
        void destructionSequenceFinished();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    private slots:
        void UpdateAnimation();
        void ProcessAudioBuffer(); // <<< Dodano
        void AudioDecodingFinished() const; // <<< Dodano
        void HandleMediaPlayerError() const; // <<< Dodano
        void HandleAudioDecoderError(); // <<< Dodano
        void PlayKonamiHint() const;
        void HandlePlayerStateChanged(QMediaPlayer::State state);
        void StartDestructionSequence();
        void SimulateClick();


private:
    void SetupSphereGeometry(int rings, int sectors);
    void SetupShaders();
    void SetupAudio(); // <<< Dodano
    float CalculateRMSAmplitude(const QAudioBuffer& buffer) const; // <<< Dodano
    void TriggerImpactAtScreenPos(const QPoint& screen_position);

    bool is_first_time_;

    QTimer timer_;
    float time_value_;

    QOpenGLShaderProgram *shader_program_;
    QOpenGLBuffer vbo_;
    QOpenGLVertexArrayObject vao_;

    QMatrix4x4 projection_matrix_;
    QMatrix4x4 view_matrix_;
    QMatrix4x4 model_matrix_;

    QVector3D camera_position_;
    float camera_distance_;

    QPoint last_mouse_position_;
    bool mouse_pressed_;
    QQuaternion rotation_;

    std::vector<GLfloat> vertices_;
    int vertex_count_;

    bool closable_;

    QVector3D angular_velocity_;
    float damping_factor_;

    QElapsedTimer elapsed_timer_;
    float last_frame_time_secs_;

    // --- Audio ---
    QMediaPlayer *media_player_; // <<< Dodano
    QAudioDecoder *audio_decoder_; // <<< Dodano
    std::vector<float> audio_amplitudes_; // <<< Dodano: Przechowuje znormalizowane amplitudy RMS dla każdego bufora
    QAudioFormat audio_format_; // <<< Dodano: Przechowuje format dekodowanego audio
    qint64 audio_buffer_duration_ms_; // <<< Dodano: Czas trwania jednego bufora audio w ms
    float current_audio_amplitude_; // <<< Dodano: Aktualna amplituda do przekazania do shadera
    float target_audio_amplitude_;
    bool audio_ready_; // <<< Dodano: Flaga wskazująca, czy dane audio są gotowe

    std::vector<ImpactInfo> impacts_;
    int next_impact_index_;

    // --- Konami Code ---
    std::deque<int> key_sequence_;
    const std::vector<int> konami_code_ = {
        Qt::Key_Up, Qt::Key_Down, Qt::Key_Up, Qt::Key_Down,
        Qt::Key_Left, Qt::Key_Right, Qt::Key_Left, Qt::Key_Right,
        Qt::Key_B, Qt::Key_A, Qt::Key_Return
    };
    QTimer* hint_timer_;

    bool is_destroying_;
    float destruction_progress_;
    const QString kGoodbyeSoundFilename = "goodbye.wav";

    QTimer* click_simulation_timer_;
};

#endif // FLOATING_ENERGY_SPHERE_WIDGET_H