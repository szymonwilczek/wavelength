#include "gl_transition_widget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QApplication>
#include <QScreen>

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

    // Pobierz współczynnik skalowania pikseli dla ekranu
    qreal devicePixelRatio = QApplication::primaryScreen()->devicePixelRatio();

    // Rozmiar widgetu w pikselach urządzenia
    QSize pixmapSize = size() * devicePixelRatio;

    // Renderowanie bieżącego widgetu do pixmap z wysoką jakością
    m_currentPixmap = QPixmap(pixmapSize);
    m_currentPixmap.fill(Qt::transparent);
    m_currentPixmap.setDevicePixelRatio(devicePixelRatio);

    QPainter currentPainter(&m_currentPixmap);
    currentPainter.setRenderHints(QPainter::Antialiasing |
                                 QPainter::TextAntialiasing |
                                 QPainter::SmoothPixmapTransform |
                                 QPainter::HighQualityAntialiasing);
    currentWidget->render(&currentPainter, QPoint(), QRegion(), QWidget::DrawChildren | QWidget::DrawWindowBackground);
    currentPainter.end();

    // Renderowanie następnego widgetu do pixmap z wysoką jakością
    m_nextPixmap = QPixmap(pixmapSize);
    m_nextPixmap.fill(Qt::transparent);
    m_nextPixmap.setDevicePixelRatio(devicePixelRatio);

    QPainter nextPainter(&m_nextPixmap);
    nextPainter.setRenderHints(QPainter::Antialiasing |
                              QPainter::TextAntialiasing |
                              QPainter::SmoothPixmapTransform |
                              QPainter::HighQualityAntialiasing);
    nextWidget->render(&nextPainter, QPoint(), QRegion(), QWidget::DrawChildren | QWidget::DrawWindowBackground);
    nextPainter.end();
}

void GLTransitionWidget::startTransition(int duration)
{
    m_offset = 0.0f;
    m_animation->setDuration(duration);
    m_animation->setStartValue(0.0f);
    m_animation->setEndValue(1.0f);
    m_animation->start();
}

void GLTransitionWidget::setOffset(float offset)
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

    int w = width();
    int h = height();

    // Rysuje bieżący widget przesuwający się w lewo
    int currentX = -w * m_offset;
    painter.drawPixmap(currentX, 0, w, h, m_currentPixmap);

    // Rysuje następny widget wjeżdżający od prawej
    int nextX = w * (1.0 - m_offset);
    painter.drawPixmap(nextX, 0, w, h, m_nextPixmap);
}