#include "dot_animation.h"
#include "constants.h"
#include <QPainter>
#include <QLinearGradient>
#include <QColor>
#include <QPaintEvent>
#include <QTimer>
#include <QDateTime>
#include <QDebug>

DotAnimation::DotAnimation(QWidget *parent)
    : QWidget(parent), breathePhase(0), numDots(100),
      springStiffness(DEFAULT_SPRING_STIFFNESS),
      dampingFactor(DEFAULT_DAMPING_FACTOR),
      isWindowTracking(false)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &DotAnimation::updateAnimation);
    timer->start(1000 / 60); // ~ 60 FPS

    initializeDots();

    setMinimumSize(400, 400);

    lastWindowPos = QPoint(0, 0);
    lastWindowSize = QSize(0, 0);
    lastUpdateTime = QDateTime::currentMSecsSinceEpoch();

    QTimer::singleShot(100, this, &DotAnimation::setupEventTracking);
}

DotAnimation::~DotAnimation() {
    timer->stop();
}

void DotAnimation::initializeDots() {
    dots.resize(numDots);

    for (int i = 0; i < numDots; ++i) {
        float angle = i * (2 * M_PI / numDots);
        float baseRadius = 0.7;
        dots[i] = Dot(angle, baseRadius, i, numDots);
    }
}

void DotAnimation::setupEventTracking() {
    if (window()) {
        lastWindowPos = window()->pos();
        lastWindowSize = window()->size();
        window()->installEventFilter(this);
        isWindowTracking = true;
    }
}

bool DotAnimation::eventFilter(QObject *obj, QEvent *event) {
    if (isWindowTracking && obj == window()) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 deltaTime = qMax(static_cast<qint64>(1), currentTime - lastUpdateTime);

        if (event->type() == QEvent::Move) {
            QPoint currentPos = window()->pos();
            QPoint delta = currentPos - lastWindowPos;

            if (!delta.isNull()) {
                float velX = -delta.x() / static_cast<float>(deltaTime) * WINDOW_MOVE_FORCE_FACTOR;
                float velY = -delta.y() / static_cast<float>(deltaTime) * WINDOW_MOVE_FORCE_FACTOR;

                applyForceToAllDots(velX, velY);
                lastWindowPos = currentPos;
                lastUpdateTime = currentTime;
            }
        }
        else if (event->type() == QEvent::Resize) {
            QSize currentSize = window()->size();
            QSize delta(currentSize.width() - lastWindowSize.width(),
                        currentSize.height() - lastWindowSize.height());

            if (delta.width() != 0 || delta.height() != 0) {
                float velX = delta.width() / static_cast<float>(deltaTime) * WINDOW_RESIZE_FORCE_FACTOR;
                float velY = delta.height() / static_cast<float>(deltaTime) * WINDOW_RESIZE_FORCE_FACTOR;

                for (int i = 0; i < numDots; ++i) {
                    float xFactor = 0.3f + 0.7f * std::abs(dots[i].getX());
                    if ((delta.width() > 0 && dots[i].getX() < 0) ||
                        (delta.width() < 0 && dots[i].getX() > 0)) {
                        xFactor *= 0.5f;
                    }

                    float yFactor = 0.3f + 0.7f * std::abs(dots[i].getY());
                    if ((delta.height() > 0 && dots[i].getY() < 0) ||
                        (delta.height() < 0 && dots[i].getY() > 0)) {
                        yFactor *= 0.5f;
                    }

                    // Zastsuj siłę z odpowiednimi współczynnikami
                    dots[i].applyForce(velX * xFactor, velY * yFactor);
                }

                lastWindowSize = currentSize;
                lastUpdateTime = currentTime;
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

void DotAnimation::applyForceToAllDots(float forceX, float forceY) {
    for (int i = 0; i < numDots; ++i) {
        float distanceFromCenter = sqrt(dots[i].getX() * dots[i].getX() + dots[i].getY() * dots[i].getY());
        float forceFactor = 0.5f + 0.5f * distanceFromCenter;

        dots[i].applyForce(forceX * forceFactor * 0.1f, forceY * forceFactor * 0.1f);
    }
}

void DotAnimation::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0, QColor(10, 10, 25));
    gradient.setColorAt(1, QColor(30, 30, 45));
    painter.fillRect(rect(), gradient);

    int centerX = width() / 2;
    int centerY = height() / 2;
    int minDimension = std::min(width(), height()) / 2 - 40;

    for (int i = 0; i < numDots; ++i) {
        int x = centerX + minDimension * dots[i].getX();
        int y = centerY + minDimension * dots[i].getY();

        float breatheFactor = 0.15f * sin(breathePhase + i * 0.2f) + 0.85f;
        float size = dots[i].getSize() * breatheFactor;

        QColor dotColor = QColor::fromHsv(
            static_cast<int>(dots[i].getHue()),
            220,
            200 + static_cast<int>(55 * sin(breathePhase * 0.3f + i * 0.1f))
        );

        painter.setOpacity(0.7 + 0.3 * (dots[i].getY() + 1.0) / 2.0);

        painter.setBrush(dotColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(x, y), size, size);

        painter.setBrush(Qt::white);
        painter.setOpacity(0.2);
        painter.drawEllipse(QPointF(x - size/4, y - size/4), size/5, size/5);
    }
}

void DotAnimation::updateAnimation() {
    breathePhase += BREATHE_INCREMENT;
    if (breathePhase > 2 * M_PI) {
        breathePhase -= 2 * M_PI;
    }

    for (int i = 0; i < numDots; ++i) {
        dots[i].update(springStiffness, dampingFactor);
    }

    update();
}