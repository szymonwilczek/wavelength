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
    m_animation = new QPropertyAnimation(this, "offset");
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_animation, &QPropertyAnimation::finished, this, &GLTransitionWidget::transitionFinished);

    // Włącz śledzenie myszy dla płynniejszego odświeżania
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
}

GLTransitionWidget::~GLTransitionWidget()
{
    delete m_animation;
}

void GLTransitionWidget::setWidgets(QWidget *currentWidget, QWidget *nextWidget)
{
    if (!currentWidget || !nextWidget)
        return;

    const qreal devicePixelRatio = QApplication::primaryScreen()->devicePixelRatio();

    m_currentPixmap = currentWidget->grab();
    m_currentPixmap.setDevicePixelRatio(devicePixelRatio);

    m_nextPixmap = nextWidget->grab();
    m_nextPixmap.setDevicePixelRatio(devicePixelRatio);
}

void GLTransitionWidget::startTransition(const int duration)
{
    m_offset = 0.0f;
    m_animation->setDuration(duration);
    m_animation->setStartValue(0.0f);
    m_animation->setEndValue(1.0f);
    m_animation->start();
}

void GLTransitionWidget::setOffset(const float offset)
{
    if (m_offset != offset) {
        m_offset = offset;
        update();  // Wymusza przerysowanie
    }
}

void GLTransitionWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // Upewniamy się, że mamy co rysować
    if (m_currentPixmap.isNull() || m_nextPixmap.isNull())
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
    const int currentX = -w * m_offset;
    painter.drawPixmap(currentX, 0, w, h, m_currentPixmap);

    // Rysuje następny widget wjeżdżający od prawej
    const int nextX = w * (1.0 - m_offset);
    painter.drawPixmap(nextX, 0, w, h, m_nextPixmap);
}