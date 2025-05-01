#include "gl_transition_widget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QApplication>
#include <QScreen>
#include <QDebug>

GLTransitionWidget::GLTransitionWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    // Inicjalizacja animacji
    animation_ = new QPropertyAnimation(this, "offset");
    animation_->setEasingCurve(QEasingCurve::OutCubic);

    connect(animation_, &QPropertyAnimation::finished, this, &GLTransitionWidget::transitionFinished);

    // Włącz śledzenie myszy dla płynniejszego odświeżania
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
}

GLTransitionWidget::~GLTransitionWidget()
{
    delete animation_;
}

void GLTransitionWidget::SetWidgets(QWidget *current_widget, QWidget *next_widget)
{
    if (!current_widget || !next_widget)
        return;

    const qreal device_pixel_ratio = QApplication::primaryScreen()->devicePixelRatio();

    current_pixmap_ = current_widget->grab();
    current_pixmap_.setDevicePixelRatio(device_pixel_ratio);

    next_pixmap_ = next_widget->grab();
    next_pixmap_.setDevicePixelRatio(device_pixel_ratio);
}

void GLTransitionWidget::StartTransition(const int duration)
{
    offset_ = 0.0f;
    animation_->setDuration(duration);
    animation_->setStartValue(0.0f);
    animation_->setEndValue(1.0f);
    animation_->start();
}

void GLTransitionWidget::SetOffset(const float offset)
{
    if (offset_ != offset) {
        offset_ = offset;
        update();  // Wymusza przerysowanie
    }
}

void GLTransitionWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // Upewniamy się, że mamy co rysować
    if (current_pixmap_.isNull() || next_pixmap_.isNull())
        return;

    QPainter painter(this);

    // Włącz wszystkie opcje wysokiej jakości
    painter.setRenderHints(QPainter::Antialiasing |
                          QPainter::TextAntialiasing |
                          QPainter::SmoothPixmapTransform |
                          QPainter::HighQualityAntialiasing);

    const int w = width();
    const int h = height();

    // Rysuje bieżący widget przesuwający się w lewo
    const int current_x = -w * offset_;
    painter.drawPixmap(current_x, 0, w, h, current_pixmap_);

    // Rysuje następny widget wjeżdżający od prawej
    const int next_x = w * (1.0 - offset_);
    painter.drawPixmap(next_x, 0, w, h, next_pixmap_);
}