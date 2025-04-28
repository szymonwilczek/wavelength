#ifndef GL_TRANSITION_WIDGET_H
#define GL_TRANSITION_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QPropertyAnimation>

class GLTransitionWidget final : public QOpenGLWidget
{
    Q_OBJECT
    Q_PROPERTY(float offset READ offset WRITE setOffset)

public:
    explicit GLTransitionWidget(QWidget *parent = nullptr);
    ~GLTransitionWidget() override;

    // Ustawia widgety do animacji
    void setWidgets(QWidget *currentWidget, QWidget *nextWidget);

    // Rozpoczyna animację przejścia
    void startTransition(int duration);

    // Właściwość do animacji - offset przesunięcia
    float offset() const { return m_offset; }
    void setOffset(float offset);

    signals:
        void transitionFinished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // Obrazy zamiast tekstur OpenGL
    QPixmap m_currentPixmap;
    QPixmap m_nextPixmap;

    // Offset animacji (0.0 - 1.0)
    float m_offset = 0.0f;

    // Animacja
    QPropertyAnimation *m_animation = nullptr;
};

#endif // GL_TRANSITION_WIDGET_H