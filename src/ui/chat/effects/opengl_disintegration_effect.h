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

class OpenGLDisintegration final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit OpenGLDisintegration(QWidget* parent = nullptr);

    ~OpenGLDisintegration() override;

    void SetSourcePixmap(const QPixmap& pixmap);

    qreal GetProgress() const { return progress_; }
    
    void SetProgress(qreal progress);

    void StartAnimation(int duration = 1000);

    void StopAnimation() const;

signals:
    void animationFinished();

protected:
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

private slots:
    void UpdateAnimation();

private:
    void CreateParticleGrid(int cols, int rows);

    QPixmap source_pixmap_;
    qreal progress_;
    QOpenGLTexture* texture_;
    QOpenGLShaderProgram* shader_program_;
    QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vbo_;
    QVector<float> vertices_;
    QTimer* timer_;
    QTime animation_start_;
    int animation_duration_{};
};

#endif // OPENGL_DISINTEGRATION_H