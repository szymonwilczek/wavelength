#include "blob_animation.h"
#include <QPainter>
#include <QDebug>
#include <QElapsedTimer>

BlobAnimation::BlobAnimation(QWidget *parent)
    : QWidget(parent)
{
    m_params.blobRadius = 250.0f;
    m_params.numPoints = 20;
    m_params.borderColor = QColor(70, 130, 180);
    m_params.glowRadius = 10;
    m_params.borderWidth = 6;
    m_params.gridSpacing = 20;

    m_idleParams.waveAmplitude = 1.7;
    m_idleParams.waveFrequency = 2.0;

    m_idleState = std::make_unique<IdleState>();
    m_movingState = std::make_unique<MovingState>();
    m_resizingState = std::make_unique<ResizingState>();
    m_currentBlobState = m_idleState.get();

    m_controlPoints.reserve(m_params.numPoints);
    m_targetPoints.reserve(m_params.numPoints);
    m_velocity.reserve(m_params.numPoints);
    m_originalControlPoints.reserve(m_params.numPoints);
    m_targetIdlePoints.reserve(m_params.numPoints);

    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, true);

    initializeBlob();

    if (window()) {
        m_lastWindowPos = window()->pos();

        window()->installEventFilter(this);
    }

    m_animationTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_animationTimer, &QTimer::timeout, this, &BlobAnimation::updateAnimation);
    m_animationTimer.start(16); // ~60 FPS
}

BlobAnimation::~BlobAnimation() {
    m_animationTimer.stop();
    m_idleTimer.stop();
    m_stateResetTimer.stop();
}

void BlobAnimation::initializeBlob() {
    m_physics.initializeBlob(m_controlPoints, m_targetPoints, m_velocity,
                           m_blobCenter, m_params, width(), height());

    m_precalcMinDistance = m_params.blobRadius * m_physicsParams.minNeighborDistance;
    m_precalcMaxDistance = m_params.blobRadius * m_physicsParams.maxNeighborDistance;
}

void BlobAnimation::paintEvent(QPaintEvent *event) {
    QRectF blobRect = calculateBlobBoundingRect();
    if (!event->rect().intersects(blobRect.toRect())) {
        return;
    }

    QPainter painter(this);

    if (m_currentState == BlobConfig::MOVING || m_currentState == BlobConfig::RESIZING) {
        painter.setRenderHint(QPainter::Antialiasing, false);
    } else {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    }

    m_renderer.renderBlob(painter, m_controlPoints, m_blobCenter,
                        m_params, width(), height());
}

QRectF BlobAnimation::calculateBlobBoundingRect() {
    if (m_controlPoints.empty()) {
        return QRectF(0, 0, width(), height());
    }

    qreal minX = std::numeric_limits<qreal>::max();
    qreal minY = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::lowest();
    qreal maxY = std::numeric_limits<qreal>::lowest();

    for (const auto& point : m_controlPoints) {
        minX = std::min(minX, point.x());
        minY = std::min(minY, point.y());
        maxX = std::max(maxX, point.x());
        maxY = std::max(maxY, point.y());
    }

    int margin = m_params.borderWidth + m_params.glowRadius + 5;
    return QRectF(minX - margin, minY - margin,
                 maxX - minX + 2*margin, maxY - minY + 2*margin);
}

void BlobAnimation::updateAnimation() {

    if (window() && m_currentState != BlobConfig::RESIZING) {
        QPointF currentWindowPos = window()->pos();
        if (currentWindowPos != m_lastWindowPos) {
            QVector2D windowVelocity = m_physics.calculateWindowVelocity(currentWindowPos);
            if (windowVelocity.length() > 0.2) {
                switchToState(BlobConfig::MOVING);
                m_movingState->applyInertiaForce(m_velocity, m_blobCenter, m_controlPoints,
                                              m_params.blobRadius, windowVelocity);
            }
            m_physics.setLastWindowPos(currentWindowPos);
            m_lastWindowPos = currentWindowPos;
        }
    }

    if (m_inTransitionToIdle) {
        handleIdleTransition();
        update();
        return;
    }

    if (m_currentState == BlobConfig::IDLE) {
        if (m_currentBlobState) {
            m_currentBlobState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
        }
    } else if (m_currentState == BlobConfig::MOVING || m_currentState == BlobConfig::RESIZING) {
        if (m_currentBlobState) {
            m_currentBlobState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
        }
    }

    updatePhysics();

    update();
}

void BlobAnimation::updatePhysics() {
    m_physics.updatePhysics(m_controlPoints, m_targetPoints, m_velocity,
                          m_blobCenter, m_params, m_physicsParams, this);

    int padding = m_params.borderWidth + m_params.glowRadius;
    m_physics.handleBorderCollisions(m_controlPoints, m_velocity, m_blobCenter,
                                   width(), height(), m_physicsParams.restitution, padding);

    m_physics.smoothBlobShape(m_controlPoints);

    double minDistance = m_params.blobRadius * m_physicsParams.minNeighborDistance;
    double maxDistance = m_params.blobRadius * m_physicsParams.maxNeighborDistance;
    m_physics.constrainNeighborDistances(m_controlPoints, m_velocity, minDistance, maxDistance);
}

void BlobAnimation::handleIdleTransition() {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsedMs = currentTime - m_transitionToIdleStartTime;

        if (elapsedMs >= m_transitionToIdleDuration) {

            m_inTransitionToIdle = false;
            m_currentBlobState = m_idleState.get();

            for (auto& vel : m_velocity) {
                vel *= 0.7;
            }

            m_idleState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
        } else {
            double progress = elapsedMs / (double)m_transitionToIdleDuration;

            double easedProgress = progress;

            if (progress < 0.3) {
                easedProgress = progress * (0.7 + progress * 0.3);
            } else if (progress > 0.7) {
                double t = 1.0 - progress;
                easedProgress = 1.0 - t * (0.7 + t * 0.3);
            }

            m_blobCenter = QPointF(
                m_originalBlobCenter.x() * (1.0 - easedProgress) + m_targetIdleCenter.x() * easedProgress,
                m_originalBlobCenter.y() * (1.0 - easedProgress) + m_targetIdleCenter.y() * easedProgress
            );

            for (size_t i = 0; i < m_controlPoints.size(); ++i) {
                m_controlPoints[i] = QPointF(
                    m_originalControlPoints[i].x() * (1.0 - easedProgress) + m_targetIdlePoints[i].x() * easedProgress,
                    m_originalControlPoints[i].y() * (1.0 - easedProgress) + m_targetIdlePoints[i].y() * easedProgress
                );
            }

            double dampingFactor = pow(0.98, 1.0 + 3.0 * easedProgress);
            for (size_t i = 0; i < m_velocity.size(); ++i) {
                m_velocity[i] *= dampingFactor;
            }

            double idleBlendFactor = easedProgress * 0.4;

            std::vector<QPointF> tempVelocity = m_velocity;
            QPointF tempCenter = m_blobCenter;
            std::vector<QPointF> tempPoints = m_controlPoints;

            m_idleState->apply(tempPoints, tempVelocity, tempCenter, m_params);

            for (size_t i = 0; i < m_velocity.size(); ++i) {
                m_velocity[i] += (tempVelocity[i] - m_velocity[i]) * idleBlendFactor;
            }

            update();
            return;
        }
}


void BlobAnimation::resizeEvent(QResizeEvent *event) {
    switchToState(BlobConfig::RESIZING);

    m_resizingState->handleResize(m_controlPoints, m_targetPoints, m_velocity,
                                m_blobCenter, event->oldSize(), event->size());

    m_renderer.resetGridBuffer();

    QWidget::resizeEvent(event);
}

bool BlobAnimation::event(QEvent *event) {
        if (event->type() == QEvent::Resize) {
        switchToState(BlobConfig::RESIZING);
    }

    return QWidget::event(event);
}

void BlobAnimation::switchToState(BlobConfig::AnimationState newState) {
    static qint64 lastStateChangeTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (m_currentState == BlobConfig::RESIZING && newState == BlobConfig::MOVING) {
        constexpr qint64 RESIZE_TO_MOVE_COOLDOWN = 650; // ms
        if (currentTime - lastStateChangeTime < RESIZE_TO_MOVE_COOLDOWN) {
            return;
        }
    }


    if (m_currentState != newState) {
        qDebug() << "State change from" << m_currentState << "to" << newState;
        lastStateChangeTime = currentTime;

        if (newState == BlobConfig::IDLE &&
            (m_currentState == BlobConfig::MOVING || m_currentState == BlobConfig::RESIZING)) {


            m_originalControlPoints = m_controlPoints;
            m_originalVelocities = m_velocity;
            m_originalBlobCenter = m_blobCenter;

            QPointF targetCenter = QPointF(width() / 2.0, height() / 2.0);

            double currentRadius = 0.0;
            for (const auto& point : m_controlPoints) {
                currentRadius += QVector2D(point - m_blobCenter).length();
            }
            currentRadius /= m_controlPoints.size();

            double radiusRatio = currentRadius / m_params.blobRadius;

            std::vector<QPointF> targetPoints(m_controlPoints.size());

            for (size_t i = 0; i < m_controlPoints.size(); ++i) {
                QPointF relativePos = m_controlPoints[i] - m_blobCenter;
                if (radiusRatio > 0.1) {
                    relativePos = relativePos / radiusRatio;
                }
                targetPoints[i] = targetCenter + relativePos;
            }

            m_targetIdlePoints = targetPoints;
            m_targetIdleCenter = targetCenter;

            m_inTransitionToIdle = true;
            m_transitionToIdleStartTime = QDateTime::currentMSecsSinceEpoch();
            m_transitionToIdleDuration = 700;

            if (m_transitionToIdleTimer) {
                m_transitionToIdleTimer->stop();
            }

            m_currentState = BlobConfig::IDLE;

            for (auto& vel : m_velocity) {
                vel *= 0.8;
            }

            return;
        }

        m_currentState = newState;

        switch (m_currentState) {
            case BlobConfig::IDLE:
                m_currentBlobState = m_idleState.get();
                break;
            case BlobConfig::MOVING:
                m_currentBlobState = m_movingState.get();
                break;
            case BlobConfig::RESIZING:
                m_currentBlobState = m_resizingState.get();
                break;
        }

        if (newState != BlobConfig::IDLE) {
            m_stateResetTimer.stop();
        }

        if (newState == BlobConfig::MOVING || newState == BlobConfig::RESIZING) {
            m_stateResetTimer.start(1000);
        }
    }
}

void BlobAnimation::applyForces(const QVector2D &force) {
    m_currentBlobState->applyForce(force, m_velocity, m_blobCenter,
                                 m_controlPoints, m_params.blobRadius);
}

void BlobAnimation::applyIdleEffect() {
    m_idleState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
}

void BlobAnimation::setBackgroundColor(const QColor &color) {
    m_params.backgroundColor = color;
    m_renderer.resetGridBuffer();
    update();
}

void BlobAnimation::setGridColor(const QColor &color) {
    m_params.gridColor = color;
    m_renderer.resetGridBuffer();
    update();
}

void BlobAnimation::setGridSpacing(int spacing) {
    m_params.gridSpacing = spacing;
    m_renderer.resetGridBuffer();
    update();
}

bool BlobAnimation::eventFilter(QObject *watched, QEvent *event) {

    if (watched == window() && event->type() == QEvent::Move) {
        static QElapsedTimer throttleTimer;
        static bool firstEvent = true;

        if (firstEvent) {
            throttleTimer.start();
            firstEvent = false;
            return QWidget::eventFilter(watched, event);
        }

        if (throttleTimer.elapsed() < 33) {
            return QWidget::eventFilter(watched, event);
        }
        throttleTimer.restart();

        if (m_currentState == BlobConfig::RESIZING) {
            qDebug() << "Ignorowanie Move podczas stanu RESIZING";
            return QWidget::eventFilter(watched, event);
        }

        QPointF currentWindowPos = window()->pos();
        QVector2D windowVelocity = m_physics.calculateWindowVelocity(currentWindowPos);

        if (windowVelocity.length() > 0.2) {

            switchToState(BlobConfig::MOVING);

            m_movingState->applyInertiaForce(m_velocity, m_blobCenter, m_controlPoints,
                                         m_params.blobRadius, windowVelocity);

            for (size_t i = 0; i < m_controlPoints.size(); ++i) {
                double randX = (qrand() % 100 - 50) / 500.0;
                double randY = (qrand() % 100 - 50) / 500.0;
                QPointF randomVariation(randX, randY);

                QPointF force(-windowVelocity.x() * 0.002, -windowVelocity.y() * 0.002);
                force += randomVariation;
                m_velocity[i] += force;
            }

            m_stateResetTimer.stop();
            m_stateResetTimer.start(1000);
        }

        m_physics.setLastWindowPos(currentWindowPos);

        updateAnimation();

    }

    return QWidget::eventFilter(watched, event);
}