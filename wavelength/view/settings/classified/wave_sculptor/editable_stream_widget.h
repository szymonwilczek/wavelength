#ifndef EDITABLE_STREAM_WIDGET_H
#define EDITABLE_STREAM_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QVector>
#include <QPointF>
#include <QTimer>
#include <QVector2D>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QLabel>

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

    // Publiczne metody do wywoływania symulacji (bez zmian)
    void triggerGlitch(qreal intensity);
    void triggerReceivingAnimation();
    void returnToIdleAnimation();

    // Settery i gettery (bez zmian)
    qreal waveAmplitude() const { return m_waveAmplitude; }
    void setWaveAmplitude(qreal amplitude) { m_waveAmplitude = amplitude; update(); }

    qreal waveFrequency() const { return m_waveFrequency; }
    void setWaveFrequency(qreal frequency) { m_waveFrequency = frequency; update(); }

    qreal waveSpeed() const { return m_waveSpeed; }
    void setWaveSpeed(qreal speed) { m_waveSpeed = speed; }

    qreal glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(qreal intensity) { m_glitchIntensity = intensity; update(); }

    qreal waveThickness() const { return m_waveThickness; }
    void setWaveThickness(qreal thickness) { m_waveThickness = thickness; update(); }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    // Obsługa myszy i klawiatury
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    // Rysowanie dodatkowych elementów interfejsu
    void paintEvent(QPaintEvent* event) override;

signals:
    // Sygnały informujące o stanie edycji
    void editingStarted();
    void editingFinished();

private slots:
    void updateAnimation();
    void updateEditHelpText();

private:
    // Inicjalizacja punktów kontrolnych
    void initializeControlPoints();

    // Znajdowanie najbliższego punktu kontrolnego do pozycji kursora
    int findClosestControlPoint(const QPointF& pos);

    // Aktualizacja tekstury waveform na podstawie punktów kontrolnych
    void updateWaveTexture();

    // Konwersja między współrzędnymi okna a przestrzenią OpenGL
    QPointF windowToGL(const QPoint& windowPos);
    QPoint glToWindow(const QPointF& glPos);

    void resetControlPoints();

    // Parametry fali
    qreal m_waveAmplitude;
    qreal m_waveFrequency;
    qreal m_waveSpeed;
    qreal m_glitchIntensity;
    qreal m_waveThickness;
    qreal m_timeOffset;

    // Timery
    QTimer* m_animTimer;
    QTimer* m_glitchDecayTimer;
    QTimer* m_helpTextTimer;

    // OpenGL
    QOpenGLShaderProgram* m_shaderProgram;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLTexture* m_waveTexture;  // Tekstura z kształtem fali
    bool m_initialized;

    // Punkty kontrolne do edycji
    QVector<QPointF> m_controlPoints;
    int m_numControlPoints;
    int m_selectedPointIndex;
    bool m_isEditing;

    // Rozmiar tekstury fali
    static const int TEXTURE_SIZE = 512;

    // Informacja pomocnicza
    QLabel* m_helpLabel;
    int m_helpTextOpacity;
    bool m_helpTextVisible;
};

#endif // EDITABLE_STREAM_WIDGET_H