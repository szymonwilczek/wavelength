#include "dot_animation.h"
#include "constants.h"
#include <QPainter>
#include <QLinearGradient>
#include <QColor>
#include <QPaintEvent>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>

DotAnimation::DotAnimation(QWidget *parent)
    : QWidget(parent), numDots(125), breathePhase(0),
      springStiffness(DEFAULT_SPRING_STIFFNESS),
      dampingFactor(DEFAULT_DAMPING_FACTOR),
      isWindowTracking(false),
      idleAnimationActive(true),
      idleAnimationPhase(0.0f),
      lastUserActionTime(0),
      strongPulseActive(false),
      strongPulseTimer(0),
      forceAngle(0.0f),
      forceWidth(0.5f),
forceDirection(1),
forceSpeed(0.015f),
targetForceAngle(0.0f),
moveToNewTarget(false),
lastTargetAngle(0.0f),
idleAnimationIntensity(1.0f)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &DotAnimation::updateAnimation);
    timer->start(1000 / 60); 

    idleAnimationTimer = new QTimer(this);
    idleAnimationTimer->setSingleShot(true);
    connect(idleAnimationTimer, &QTimer::timeout, this, &DotAnimation::resumeIdleAnimation);

    initializeDots();

    setMinimumSize(400, 400);

    lastWindowPos = QPoint(0, 0);
    lastWindowSize = QSize(0, 0);
    lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    lastUserActionTime = lastUpdateTime;

    forceAngle = 0.0f;
    forceWidth = 0.5f;

    QTimer::singleShot(100, this, &DotAnimation::setupEventTracking);
}

DotAnimation::~DotAnimation() {
    timer->stop();
    idleAnimationTimer->stop();
    delete idleAnimationTimer;
}

void DotAnimation::initializeDots() {
    dots.resize(numDots);

    for (int i = 0; i < numDots; ++i) {
        float baseAngle = i * 2 * M_PI / numDots;

        float angleDistortion = sin(baseAngle * 2) * 0.1f +
                       sin(baseAngle * 5) * 0.05f;



        float finalAngle = baseAngle + angleDistortion;

        float baseRadius = 0.8f;
        float radiusDistortion = sin(baseAngle * 2) * 0.1f +
                                sin(baseAngle * 4) * 0.07f +
                                sin(baseAngle * 6) * 0.03f;

        float finalRadius = baseRadius + radiusDistortion;

        dots[i] = Dot(finalAngle, finalRadius, i, numDots);

        float randomValue = QRandomGenerator::global()->bounded(1.0f);
        float mappedValue = 0.6f + 0.3f * (randomValue * randomValue);
        dots[i].setSize(dots[i].getSize() * mappedValue);
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
                
                pauseIdleAnimation();

                float velX = -delta.x() / static_cast<float>(deltaTime) * WINDOW_MOVE_FORCE_FACTOR;
                float velY = -delta.y() / static_cast<float>(deltaTime) * WINDOW_MOVE_FORCE_FACTOR;

                applyForceToAllDots(velX, velY);
                lastWindowPos = currentPos;
                lastUpdateTime = currentTime;
                lastUserActionTime = currentTime;
            }
        }
        else if (event->type() == QEvent::Resize) {
            QSize currentSize = window()->size();
            QSize delta(currentSize.width() - lastWindowSize.width(),
                        currentSize.height() - lastWindowSize.height());

            if (delta.width() != 0 || delta.height() != 0) {
                
                pauseIdleAnimation();

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

                    
                    dots[i].applyForce(velX * xFactor, velY * yFactor);
                }

                lastWindowSize = currentSize;
                lastUpdateTime = currentTime;
                lastUserActionTime = currentTime;
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


        QColor dotColor;
        if (useDualColor) {
            dotColor = (i % 2 == 0) ? primaryColor : secondaryColor;
        } else {
            dotColor = primaryColor;
        }

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

    if (idleAnimationActive) {

        if (idleAnimationIntensity < 1.0f) {
            idleAnimationIntensity += 0.01f;
        }

        idleAnimationPhase += 0.01f;
        if (idleAnimationPhase > 2 * M_PI) {
            idleAnimationPhase -= 2 * M_PI;
        }

        float angleDiffToTarget = fabs(forceAngle - targetForceAngle);
        if (angleDiffToTarget > M_PI) angleDiffToTarget = 2 * M_PI - angleDiffToTarget;

        if (angleDiffToTarget < 0.05f || moveToNewTarget) {
            targetForceAngle = QRandomGenerator::global()->bounded(2.0f * M_PI);
            moveToNewTarget = false;
        }

        if (!moveToNewTarget && QRandomGenerator::global()->bounded(1000) < 1) {
            float minAngleDiff = M_PI/2;
            float newTarget = 0.0f;
            bool validTarget = false;

            for (int i = 0; i < 5; i++) {
                newTarget = QRandomGenerator::global()->bounded(2.0f * M_PI);

                float diff = fabs(newTarget - forceAngle);
                if (diff > M_PI) diff = 2 * M_PI - diff;

                if (diff > minAngleDiff) {
                    validTarget = true;
                    break;
                }
            }

            if (validTarget) {
                targetForceAngle = newTarget;
                lastTargetAngle = forceAngle;
                moveToNewTarget = true;
            }
        }

        if (moveToNewTarget) {
            float diff = targetForceAngle - forceAngle;
            if (diff > M_PI) diff -= 2 * M_PI;
            if (diff < -M_PI) diff += 2 * M_PI;

            float stepSize = 0.01f;

            if (fabs(diff) < stepSize) {
                forceAngle = targetForceAngle;
                moveToNewTarget = false;
            } else {
                forceAngle += (diff > 0) ? stepSize : -stepSize;
            }
        } else {
            forceAngle += 0.005f;
        }

        if (forceAngle > 2 * M_PI) forceAngle -= 2 * M_PI;
        if (forceAngle < 0) forceAngle += 2 * M_PI;


        if (QRandomGenerator::global()->bounded(100) < 2) {
            forceWidth = 0.15f + QRandomGenerator::global()->bounded(0.2f);
        }

        if (strongPulseActive) {
            strongPulseTimer--;
            if (strongPulseTimer <= 0) {
                strongPulseActive = false;
            }
        } else if (QRandomGenerator::global()->bounded(1000) < 5) {  
            strongPulseActive = true;
            strongPulseTimer = 10;
        }


        for (int i = 0; i < numDots; ++i) {
            float dotX = dots[i].getX();
            float dotY = dots[i].getY();

            float dotAngle = atan2(dotY, dotX);
            if (dotAngle < 0) dotAngle += 2 * M_PI;

            float angleDiff = fabs(dotAngle - forceAngle);
            if (angleDiff > M_PI) angleDiff = 2 * M_PI - angleDiff;

            if (angleDiff < forceWidth) {
                float distanceFromCenter = sqrt(dotX * dotX + dotY * dotY);

                float wavePhase = idleAnimationPhase * 4.0f - distanceFromCenter * 10.0f;
                float waveStrength = 0.5f + 0.5f * sin(wavePhase);

                float distanceFactor = 1.0f - 0.3f * distanceFromCenter;
                distanceFactor = std::max(0.0f, distanceFactor);

                float directionFactor = 1.0f - angleDiff / forceWidth;
                directionFactor = directionFactor * directionFactor;

                float forceMagnitude = 0.001f * waveStrength * distanceFactor * directionFactor * idleAnimationIntensity;


                if (strongPulseActive) {
                    forceMagnitude *= 1.2f;
                }

                float forceX = dotX * forceMagnitude;
                float forceY = dotY * forceMagnitude;

                dots[i].applyForce(forceX, forceY);
            }
        }
    }


    for (int i = 0; i < numDots; ++i) {
        dots[i].update(springStiffness, dampingFactor);
    }

    update();
}

void DotAnimation::pauseIdleAnimation() {
    idleAnimationActive = false;
    idleAnimationTimer->start(3000);
}


void DotAnimation::resumeIdleAnimation() {
    idleAnimationActive = true;
    idleAnimationIntensity = 0.0f;
}