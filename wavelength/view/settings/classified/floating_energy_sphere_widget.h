#ifndef FLOATING_ENERGY_SPHERE_WIDGET_H
#define FLOATING_ENERGY_SPHERE_WIDGET_H

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

class FloatingEnergySphereWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit FloatingEnergySphereWidget(QWidget *parent = nullptr);
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

private:
    void setupSphereGeometry(int rings, int sectors);
    void setupShaders();

    QTimer m_timer;
    float m_timeValue; // Używane w shaderach do animacji

    QOpenGLShaderProgram *m_program;
    QOpenGLBuffer m_vbo; // Vertex Buffer Object (tylko pozycje)
    // Usunięto m_ebo
    QOpenGLVertexArrayObject m_vao;

    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_modelMatrix;

    QVector3D m_cameraPosition;
    float m_cameraDistance;

    QPoint m_lastMousePos;
    bool m_mousePressed;
    QQuaternion m_rotation;

    // Geometria - tylko wierzchołki
    std::vector<GLfloat> m_vertices;
    // Usunięto m_indices i m_indexCount
    int m_vertexCount; // Liczba wierzchołków do narysowania

    bool m_closable;

    QVector3D m_angularVelocity; // Prędkość kątowa obrotu
    float m_dampingFactor;
};

#endif // FLOATING_ENERGY_SPHERE_WIDGET_H