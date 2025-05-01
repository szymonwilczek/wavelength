#ifndef OPENGL_DISINTEGRATION_H
#define OPENGL_DISINTEGRATION_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QPixmap>
#include <QTime>
#include <QTimer>
#include <QVector>
#include <QMatrix4x4>

/**
 * @brief An OpenGL-based widget that renders a disintegration effect on a source pixmap.
 *
 * This widget uses OpenGL shaders to create a particle-based disintegration effect.
 * The source pixmap is rendered as a grid of particles that scatter and fade out
 * based on the `progress` property. The animation can be controlled manually via
 * SetProgress or automatically via StartAnimation/StopAnimation.
 */
class OpenGLDisintegration final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    /** @brief Property controlling the progress of the disintegration animation (0.0 to 1.0). Animatable. */
    Q_PROPERTY(qreal progress READ GetProgress WRITE SetProgress) // Corrected getter/setter names

public:
    /**
     * @brief Constructs an OpenGLDisintegration widget.
     * Initializes OpenGL settings (transparency, format), sets up the animation timer.
     * @param parent Optional parent widget.
     */
    explicit OpenGLDisintegration(QWidget* parent = nullptr);

    /**
     * @brief Destructor. Cleans up OpenGL resources (VAO, VBO, texture, shader program).
     */
    ~OpenGLDisintegration() override;

    /**
     * @brief Sets the source pixmap to apply the disintegration effect to.
     * Recreates the OpenGL texture on the next paintGL call.
     * @param pixmap The source QPixmap.
     */
    void SetSourcePixmap(const QPixmap& pixmap);

    /**
     * @brief Gets the current progress of the disintegration animation.
     * @return The current progress level (0.0 to 1.0).
     */
    qreal GetProgress() const { return progress_; }

    /**
     * @brief Sets the progress of the disintegration animation manually.
     * Clamps the value between 0.0 and 1.0 and triggers an update.
     * @param progress The desired progress level (0.0 to 1.0).
     */
    void SetProgress(qreal progress);

    /**
     * @brief Starts the automatic disintegration animation.
     * Resets progress to 0.0 and animates it to 1.0 over the specified duration.
     * @param duration The duration of the animation in milliseconds (default: 1000).
     */
    void StartAnimation(int duration = 1000);

    /**
     * @brief Stops the automatic disintegration animation if it's running.
     */
    void StopAnimation() const;

signals:
    /**
     * @brief Emitted when the automatic animation started by StartAnimation() completes (progress reaches 1.0).
     */
    void animationFinished();

protected:
    /**
     * @brief Initializes OpenGL resources. Called once before the first paintGL call.
     * Initializes OpenGL functions, sets clear color, compiles and links shaders,
     * creates VAO/VBO, and sets up vertex attributes.
     */
    void initializeGL() override;

    /**
     * @brief Handles resizing of the OpenGL viewport. Called whenever the widget is resized.
     * @param w The new width.
     * @param h The new height.
     */
    void resizeGL(int w, int h) override;

    /**
     * @brief Renders the OpenGL scene. Called whenever the widget needs to be repainted.
     * Clears the buffer, ensures the texture exists, binds the shader program and VAO,
     * sets uniforms (model matrix, progress, resolution, texture), enables blending,
     * and draws the particle grid.
     */
    void paintGL() override;

private slots:
    /**
     * @brief Slot called by the animation timer (timer_) to update the animation progress.
     * Calculates the new progress based on elapsed time, updates the progress_ member,
     * stops the timer if animation is complete, emits animationFinished, and triggers a repaint.
     */
    void UpdateAnimation();

private:
    /**
     * @brief Creates the vertex data for the particle grid used in the effect.
     * Generates vertex positions and texture coordinates for a grid of specified dimensions
     * and uploads the data to the VBO.
     * @param cols Number of columns in the particle grid.
     * @param rows Number of rows in the particle grid.
     */
    void CreateParticleGrid(int cols, int rows);

    /** @brief The source image that the effect is applied to. */
    QPixmap source_pixmap_;
    /** @brief Current progress level of the disintegration (0.0 to 1.0). */
    qreal progress_;
    /** @brief Pointer to the OpenGL texture created from source_pixmap_. */
    QOpenGLTexture* texture_;
    /** @brief Pointer to the compiled and linked OpenGL shader program. */
    QOpenGLShaderProgram* shader_program_;
    /** @brief OpenGL Vertex Array Object managing the vertex buffer state. */
    QOpenGLVertexArrayObject vao_;
    /** @brief OpenGL Vertex Buffer Object storing the particle grid vertex data. */
    QOpenGLBuffer vbo_;
    /** @brief Raw vertex data (position + texture coords) for the particle grid. */
    QVector<float> vertices_;
    /** @brief Timer driving the automatic animation progress. */
    QTimer* timer_;
    /** @brief Timestamp when the automatic animation started. */
    QTime animation_start_;
    /** @brief Total duration for the automatic animation in milliseconds. */
    int animation_duration_{};
};

#endif // OPENGL_DISINTEGRATION_H