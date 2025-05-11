#ifndef FLOATING_ENERGY_SPHERE_WIDGET_H
#define FLOATING_ENERGY_SPHERE_WIDGET_H

#include <deque>
#include <QAudioFormat>
#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QMediaPlayer>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QVector3D>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QTimer>
#include <QtMultimedia/QAudioDecoder>
#include <QtMultimedia/QAudioBuffer>

/** @brief Maximum number of concurrent impact effects supported by the shader. */
#define MAX_IMPACTS 5

class QOpenGLShaderProgram;
/**
 * @brief Structure holding information about a single impact effect on the sphere.
 */
struct ImpactInfo {
    /** @brief Point of impact in local sphere coordinates (normalized). */
    QVector3D point = QVector3D(0, 0, 0);
    /** @brief Time (from time_value_) when the impact started. -1.0f if inactive. */
    float start_time = -1.0f;
    /** @brief Flag indicating if this impact slot is currently active. */
    bool active = false;
};

/**
 * @brief An OpenGL widget displaying a floating, interactive energy sphere.
 *
 * This widget renders a sphere composed of points using OpenGL shaders. The sphere
 * deforms dynamically based on time, audio amplitude (visualizing audio playback),
 * and simulated impacts triggered by clicks or automatically. It supports mouse interaction
 * for rotation and zooming. It also includes an Easter egg triggered by the Konami code,
 * which initiates a destruction animation sequence with accompanying sound.
 * The widget plays different initial audio files based on whether it's the first time
 * the application is run.
 */
class FloatingEnergySphereWidget final : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    /**
     * @brief Constructs a FloatingEnergySphereWidget.
     * Initializes OpenGL settings, timers, audio components, interaction variables,
     * and sets up the window appearance (frameless, transparent background).
     * @param is_first_time Flag indicating if this is the first launch, affecting initial audio.
     * @param parent Optional parent widget.
     */
    explicit FloatingEnergySphereWidget(bool is_first_time, QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Cleans up OpenGL resources (VBO, VAO, shader), stops timers, and stops audio playback/decoding.
     */
    ~FloatingEnergySphereWidget() override;

    /**
     * @brief Sets whether the widget can be closed by the user or programmatically.
     * Used to prevent closing during the destruction animation.
     * @param closable True to allow closing, false to prevent it.
     */
    void SetClosable(bool closable);

signals:
    /** @brief Emitted when the widget is closed normally (not during destruction). */
    void widgetClosed();

    /** @brief Emitted when the Konami code sequence is successfully entered. (Deprecated, destruction starts directly) */
    void konamiCodeEntered();

    /** @brief Emitted when the destruction animation sequence (visual and audio) completes. */
    void destructionSequenceFinished();

protected:
    /**
     * @brief Initializes OpenGL resources (functions, shaders, geometry, VAO/VBO) and audio setup.
     * Called once before the first paintGL call.
     */
    void initializeGL() override;

    /**
     * @brief Handles resizing of the OpenGL viewport and updates the projection matrix.
     * Calculates the field of view dynamically to keep the sphere's clear size consistent.
     * @param w The new width.
     * @param h The new height.
     */
    void resizeGL(int w, int h) override;

    /**
     * @brief Renders the OpenGL scene. Called whenever the widget needs to be repainted.
     * Binds shaders and VAO, sets uniforms (matrices, time, audio amplitude, impact data, destruction progress),
     * and draws the sphere points.
     */
    void paintGL() override;

    /**
     * @brief Handles mouse press events. Accepts the event, but the primary logic is in mouseMoveEvent.
     * @param event The mouse event.
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief Handles mouse move events. Rotates the sphere based on mouse dragging when the left button is pressed.
     * Updates angular velocity for inertia effect.
     * @param event The mouse event.
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * @brief Handles mouse release events. Stops direct rotation tracking and allows inertia to take over.
     * @param event The mouse event.
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * @brief Handles mouse wheel events. Zooms the camera in or out by adjusting camera_distance_.
     * @param event The wheel event.
     */
    void wheelEvent(QWheelEvent *event) override;

    /**
     * @brief Handles the widget close event.
     * Stops timers and audio, emits widgetClosed() if allowed (closable_ is true), otherwise ignores the event.
     * @param event The close event.
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief Handles key press events. Detects the Konami code sequence.
     * @param event The key event.
     */
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    /**
     * @brief Slot called periodically by timer_ to update animation state.
     * Advances time, updates audio amplitude interpolation, handles rotation/inertia,
     * updates destruction progress if active, and schedules a repaint.
     */
    void UpdateAnimation();

    /**
     * @brief Slot called by the audio decoder when a new buffer of decoded audio data is ready.
     * Calculates the RMS amplitude for the buffer and stores it. Sets audio_ready_ flag.
     */
    void ProcessAudioBuffer();

    /**
     * @brief Slot called by the audio decoder when it finishes decoding the entire source file.
     */
    void AudioDecodingFinished() const;

    /**
     * @brief Slot called when the QMediaPlayer encounters an error. Logs the error.
     */
    void HandleMediaPlayerError() const;

    /**
     * @brief Slot called when the QAudioDecoder encounters an error. Logs the error.
     */
    void HandleAudioDecoderError();

    /**
     * @brief Slot called by hint_timer_ after a period of inactivity to play the Konami code audio hint.
     */
    void PlayKonamiHint() const;

    /**
     * @brief Slot called when the QMediaPlayer's state changes (e.g., Playing, Stopped).
     * Used to detect the end of the initial audio to start the hint timer, and the end
     * of the destruction audio to close the widget.
     * @param state The new media player state.
     */
    void HandlePlayerStateChanged(QMediaPlayer::State state);

    /**
     * @brief Initiates the destruction animation sequence.
     * Sets the is_destroying_ flag, resets progress, plays the destruction sound,
     * and prevents the widget from being closed prematurely.
     */
    void StartDestructionSequence();

    /**
     * @brief Slot called periodically by click_simulation_timer_ to trigger random impacts on the sphere.
     */
    void SimulateClick();

private:
    /**
     * @brief Creates the vertex data (positions) for the sphere geometry using rings and sectors.
     * Populates the vertices_ vector.
     * @param rings Number of horizontal rings.
     * @param sectors Number of vertical sectors.
     */
    void SetupSphereGeometry(int rings, int sectors);

    /**
     * @brief Compiles and links the vertex and fragment shaders into shader_program_.
     */
    void SetupShaders();

    /**
     * @brief Initializes audio components (QMediaPlayer, QAudioDecoder), connects signals,
     * selects the appropriate audio file based on is_first_time_, and starts playback/decoding.
     */
    void SetupAudio();

    /**
     * @brief Calculates the Root Mean Square (RMS) amplitude for a given audio buffer.
     * Used to quantify the energy level in the audio signal for visualization.
     * Handles different audio sample formats.
     * @param buffer The QAudioBuffer containing decoded audio data.
     * @return The calculated RMS amplitude, normalized roughly to the range [0.0, 1.0].
     */
    float CalculateRMSAmplitude(const QAudioBuffer &buffer) const;

    /**
     * @brief Triggers a visual impact effect at a point on the sphere corresponding to a screen position.
     * Performs ray casting from the screen position to find the intersection point on the sphere
     * and activates an ImpactInfo entry.
     * @param screen_position The position on the widget where the impact should originate.
     */
    void TriggerImpactAtScreenPos(const QPoint &screen_position);

    /** @brief Flag indicating if this is the first time the widget/app is run (affects initial audio). */
    bool is_first_time_;

    /** @brief Timer driving the main animation loop (UpdateAnimation). */
    QTimer timer_;
    /** @brief Time value passed to shaders, incremented in UpdateAnimation. */
    float time_value_;

    /** @brief The compiled and linked OpenGL shader program. */
    QOpenGLShaderProgram *shader_program_;
    /** @brief OpenGL Vertex Buffer Object storing sphere vertex data. */
    QOpenGLBuffer vbo_;
    /** @brief OpenGL Vertex Array Object managing vertex buffer state. */
    QOpenGLVertexArrayObject vao_;

    /** @brief Projection matrix transforming view space to clip space. */
    QMatrix4x4 projection_matrix_;
    /** @brief View matrix transforming world space to view space. */
    QMatrix4x4 view_matrix_;
    /** @brief Model matrix transforming model space to world space (includes rotation). */
    QMatrix4x4 model_matrix_;

    /** @brief Position of the camera in world space. */
    QVector3D camera_position_;
    /** @brief Distance from the camera to the origin (center of the sphere). Used for zooming. */
    float camera_distance_;

    /** @brief Stores the last mouse position during dragging for rotation calculation. */
    QPoint last_mouse_position_;
    /** @brief Flag indicating if the left mouse button is currently pressed. */
    bool mouse_pressed_;
    /** @brief Current rotation of the sphere as a quaternion. */
    QQuaternion rotation_;

    /** @brief Raw vertex data (positions) for the sphere points. */
    std::vector<GLfloat> vertices_;
    /** @brief Total number of vertices in the sphere geometry. */
    int vertex_count_;

    /** @brief Flag indicating if the widget can be closed. */
    bool closable_;

    /** @brief Current angular velocity used for inertial rotation. */
    QVector3D angular_velocity_;
    /** @brief Damping factor applied to angular velocity for inertia decay. */
    float damping_factor_;

    /** @brief Timer used for calculating delta time between animation frames. */
    QElapsedTimer elapsed_timer_;
    /** @brief Timestamp of the last animation frame in seconds. */
    float last_frame_time_secs_;

    // --- Audio ---
    /** @brief Media player for playing audio files (initial sound, hint, destruction). */
    QMediaPlayer *media_player_;
    /** @brief Decoder for extracting audio data from files for amplitude analysis. */
    QAudioDecoder *audio_decoder_;
    /** @brief Stores calculated RMS amplitude values for each decoded audio buffer. */
    std::vector<float> audio_amplitudes_;
    /** @brief Format of the decoded audio data (sample rate, channels, etc.). */
    QAudioFormat audio_format_;
    /** @brief Duration of a single audio buffer processed by the decoder, in milliseconds. */
    qint64 audio_buffer_duration_ms_;
    /** @brief Current audio amplitude value used for rendering (smoothed). */
    float current_audio_amplitude_;
    /** @brief Target audio amplitude based on the currently playing audio buffer. */
    float target_audio_amplitude_{};
    /** @brief Flag indicating if audio data has been successfully decoded and is ready for visualization. */
    bool audio_ready_;
    // --- End Audio ---

    /** @brief Stores information about active impact effects. */
    std::vector<ImpactInfo> impacts_;
    /** @brief Index of the next slot to use in the impacts_ vector (circular). */
    int next_impact_index_;

    // --- Konami Code ---
    /** @brief Stores the sequence of recently pressed keys for Konami code detection. */
    std::deque<int> key_sequence_;
    /** @brief The target key sequence for the Konami code. */
    const std::vector<int> konami_code_;
    /** @brief Timer triggering the Konami code audio hints after inactivity. */
    QTimer *hint_timer_;
    // --- End Konami Code ---

    /** @brief Flag indicating if the destruction animation is currently active. */
    bool is_destroying_;
    /** @brief Progress of the destruction animation (0.0 to 1.0). */
    float destruction_progress_;
    /** @brief Filename of the audio played during the destruction sequence. */
    const QString kGoodbyeSoundFilename = "goodbye.wav";

    /** @brief Timer triggering simulated random clicks/impacts on the sphere. */
    QTimer *click_simulation_timer_;
};

#endif // FLOATING_ENERGY_SPHERE_WIDGET_H
