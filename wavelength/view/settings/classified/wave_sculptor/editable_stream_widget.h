#ifndef EDITABLE_STREAM_WIDGET_H
#define EDITABLE_STREAM_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <QPointF>
#include <QTimer>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QVector2D> // Dodaj dla resolution

class EditableStreamWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    Q_PROPERTY(qreal waveAmplitude READ waveAmplitude WRITE setWaveAmplitude)
    Q_PROPERTY(qreal waveFrequency READ waveFrequency WRITE setWaveFrequency)
    Q_PROPERTY(qreal waveSpeed READ waveSpeed WRITE setWaveSpeed)
    Q_PROPERTY(qreal glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)
    Q_PROPERTY(qreal waveThickness READ waveThickness WRITE setWaveThickness)

public:
    explicit EditableStreamWidget(QWidget* parent = nullptr);
    ~EditableStreamWidget() override;

    // Publiczne metody do wywoływania symulacji
    void triggerGlitch(qreal intensity);
    void triggerReceivingAnimation();
    void returnToIdleAnimation();

    // Settery i gettery dla animacji fali
    qreal waveAmplitude() const { return m_waveAmplitude; }
    void setWaveAmplitude(qreal amplitude) { m_waveAmplitude = amplitude; update(); }
    qreal waveFrequency() const { return m_waveFrequency; }
    void setWaveFrequency(qreal frequency) { m_waveFrequency = frequency; update(); }
    qreal waveSpeed() const { return m_waveSpeed; }
    void setWaveSpeed(qreal speed) { m_waveSpeed = speed; } // Nie wymaga update(), bo wpływa na m_timeOffset
    qreal glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(qreal intensity) { m_glitchIntensity = intensity; update(); }
    qreal waveThickness() const { return m_waveThickness; }
    void setWaveThickness(qreal thickness) { m_waveThickness = thickness; update(); }


protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    // Usuwamy obsługę myszy na razie
    // void mousePressEvent(QMouseEvent* event) override;
    // void mouseMoveEvent(QMouseEvent* event) override;
    // void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void updateAnimation();

private:
    // Usunięto metody związane z punktami
    // void initializeWavePoints();
    // void updateVertexBuffer();
    // int findClosestPoint(const QPointF& pos);

    // Parametry fali (jak w CommunicationStream)
    qreal m_waveAmplitude;
    qreal m_waveFrequency;
    qreal m_waveSpeed;
    qreal m_glitchIntensity;
    qreal m_waveThickness;
    qreal m_timeOffset;

    QTimer* m_animTimer;
    QTimer* m_glitchDecayTimer;

    // OpenGL
    QOpenGLShaderProgram* m_shaderProgram;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vertexBuffer; // Teraz dla quada
    bool m_initialized; // Zmieniono nazwę z m_glInitialized dla spójności

    // Usunięto dane fali i edycji
    // QVector<QPointF> m_wavePoints;
    // int m_numPoints;
    // bool m_isDragging;
    // int m_draggedPointIndex;
    // qreal m_baseY;
};

#endif // EDITABLE_STREAM_WIDGET_H