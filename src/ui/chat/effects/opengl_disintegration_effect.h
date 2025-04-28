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

    void setSourcePixmap(const QPixmap& pixmap);

    qreal progress() const { return m_progress; }
    
    void setProgress(qreal progress);

    void startAnimation(int duration = 1000);

    void stopAnimation() const;

signals:
    void animationFinished();

protected:
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

private slots:
    void updateAnimation();

private:
    void createParticleGrid(int cols, int rows);

    QPixmap m_sourcePixmap;
    qreal m_progress;
    QOpenGLTexture* m_texture;
    QOpenGLShaderProgram* m_program;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;
    QVector<float> m_vertices;
    QTimer* m_timer;
    QTime m_animationStart;
    int m_animationDuration;
};

#endif // OPENGL_DISINTEGRATION_H