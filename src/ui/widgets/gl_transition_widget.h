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

    void SetWidgets(QWidget *current_widget, QWidget *next_widget);

    void StartTransition(int duration);

    float GetOffset() const { return offset_; }
    void SetOffset(float offset);

    signals:
        void transitionFinished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap current_pixmap_;
    QPixmap next_pixmap_;
    float offset_ = 0.0f;
    QPropertyAnimation *animation_ = nullptr;
};

#endif // GL_TRANSITION_WIDGET_H