#include "blob_animation.h"
#include <QPainter>
#include <QDebug>

BlobAnimation::BlobAnimation(QWidget *parent)
    : QWidget(parent)
{
    m_params.blobRadius = 300.0f;
    m_params.numPoints = 24;
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

    connect(&m_idleTimer, &QTimer::timeout, this, [this]() {
        if (m_currentState == BlobConfig::IDLE) {
            applyIdleEffect();
        }
    });
    m_idleTimer.start(16);

    connect(&m_stateResetTimer, &QTimer::timeout, this, [this]() {
        switchToState(BlobConfig::IDLE);
    });

    m_controlPoints.resize(m_params.numPoints);
    m_targetPoints.resize(m_params.numPoints);
    m_velocity.resize(m_params.numPoints);

    setAttribute(Qt::WA_TranslucentBackground);

    m_animationTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_animationTimer, &QTimer::timeout, this, &BlobAnimation::updateAnimation);
    m_animationTimer.start(16); // ~60 FPS

    initializeBlob();

    if (window()) {
        m_lastWindowPos = window()->pos();

        qDebug() << "Installing event filter on window";
        window()->installEventFilter(this);
    } else {
        qDebug() << "No window found to install event filter!";
    }

    m_windowPosCheckTimer = new QTimer(this);
    m_windowPosCheckTimer->setTimerType(Qt::PreciseTimer);
    connect(m_windowPosCheckTimer, &QTimer::timeout, this, [this]() {
        if (window()) {
            QPointF currentWindowPos = window()->pos();
            if (currentWindowPos != m_lastWindowPos) {
                qDebug() << "Window position changed (detected by timer): "
                         << m_lastWindowPos << " -> " << currentWindowPos;

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
    });
    m_windowPosCheckTimer->start(16);
}

BlobAnimation::~BlobAnimation() {
    m_animationTimer.stop();
    m_idleTimer.stop();
    m_stateResetTimer.stop();
}

void BlobAnimation::initializeBlob() {
    m_physics.initializeBlob(m_controlPoints, m_targetPoints, m_velocity,
                           m_blobCenter, m_params, width(), height());
}

void BlobAnimation::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    m_renderer.renderBlob(painter, m_controlPoints, m_blobCenter, 
                        m_params, width(), height());
}

void BlobAnimation::updateAnimation() {
    static int frameCounter = 0;
    frameCounter++;
    bool shouldLog = (frameCounter % 60 == 0);

    if (m_inTransitionToIdle) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 elapsedMs = currentTime - m_transitionToIdleStartTime;

        if (elapsedMs >= m_transitionToIdleDuration) {
            if (shouldLog) qDebug() << "Przejście do IDLE ukończone";

            m_inTransitionToIdle = false;
            m_currentBlobState = m_idleState.get();

            for (auto& vel : m_velocity) {
                vel *= 0.2;
            }
        } else {
            double progress = elapsedMs / (double)m_transitionToIdleDuration;

            double easedProgress;
            if (progress < 0.5) {
                easedProgress = 2.0 * progress * progress;
            } else {
                double t = 1.0 - progress;
                easedProgress = 1.0 - 2.0 * t * t;
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

                m_targetPoints[i] = m_controlPoints[i];
            }

            for (size_t i = 0; i < m_velocity.size(); ++i) {
                m_velocity[i] *= (1.0 - 0.05 * progress);
            }

            if (progress > 0.85) {
                double idleBlendFactor = (progress - 0.85) / 0.15; // 0->1 w ostatnich 15% przejścia

                std::vector<QPointF> tempVelocity = m_velocity;
                QPointF tempCenter = m_blobCenter;
                std::vector<QPointF> tempPoints = m_controlPoints;

                m_idleState->apply(tempPoints, tempVelocity, tempCenter, m_params);

                for (size_t i = 0; i < m_velocity.size(); ++i) {
                    m_velocity[i] = m_velocity[i] * (1.0 - idleBlendFactor) +
                                    tempVelocity[i] * idleBlendFactor * 0.3;
                }
            }

            update();
            return;
        }
    }

    if (m_currentState == BlobConfig::IDLE) {
        if (shouldLog && !m_inTransitionToIdle) qDebug() << "Animation update: IDLE state";
        if (m_currentBlobState) {
            m_currentBlobState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
        }
    }
    else if (m_currentState == BlobConfig::MOVING) {
        if (shouldLog) qDebug() << "Animation update: MOVING state";

        if (window()) {
            QVector2D windowVelocity = m_physics.getLastWindowVelocity();
            if (shouldLog) qDebug() << "Window velocity:" << windowVelocity.x() << "," << windowVelocity.y();

            if (windowVelocity.length() > 0.1) {
                if (shouldLog) qDebug() << "Applying moving state effect";
                if (m_currentBlobState) {
                    m_currentBlobState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
                }
            }
        }
    } else if (m_currentState == BlobConfig::RESIZING) {
        if (shouldLog) qDebug() << "Animation update: RESIZING state";
        if (m_currentBlobState) {
            m_currentBlobState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
        }
    }

    m_physics.updatePhysics(m_controlPoints, m_targetPoints, m_velocity,
                          m_blobCenter, m_params, m_physicsParams, this);
    
    int padding = m_params.borderWidth + m_params.glowRadius;
    m_physics.handleBorderCollisions(m_controlPoints, m_velocity, m_blobCenter, 
                                   width(), height(), m_physicsParams.restitution, padding);
    
    m_physics.smoothBlobShape(m_controlPoints);
    
    double minDistance = m_params.blobRadius * m_physicsParams.minNeighborDistance;
    double maxDistance = m_params.blobRadius * m_physicsParams.maxNeighborDistance;
    m_physics.constrainNeighborDistances(m_controlPoints, m_velocity, minDistance, maxDistance);
    
    update();
}

void BlobAnimation::resizeEvent(QResizeEvent *event) {
    switchToState(BlobConfig::RESIZING);

    m_resizingState->handleResize(m_controlPoints, m_targetPoints, m_velocity,
                                m_blobCenter, event->oldSize(), event->size());

    QWidget::resizeEvent(event);
}

bool BlobAnimation::event(QEvent *event) {
    if (event->type() == QEvent::Move) {
        qDebug() << "EVENT DETECTED: Move event";
        switchToState(BlobConfig::MOVING);

        if (window()) {
            QPointF currentWindowPos = window()->pos();
            QVector2D windowVelocity = m_physics.calculateWindowVelocity(currentWindowPos);

            qDebug() << "Window velocity:" << windowVelocity.x() << "," << windowVelocity.y()
                     << "length:" << windowVelocity.length();

            if (windowVelocity.length() > 0.2) {
                qDebug() << "Applying inertia force!";

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
            } else {
                qDebug() << "Movement too small - ignoring";
            }

            m_physics.setLastWindowPos(currentWindowPos);
        }

        updateAnimation();
    }
    else if (event->type() == QEvent::Resize) {
        qDebug() << "EVENT DETECTED: Resize event";
        switchToState(BlobConfig::RESIZING);
    }

    return QWidget::event(event);
}

void BlobAnimation::switchToState(BlobConfig::AnimationState newState) {
    if (m_currentState != newState) {
        qDebug() << "State change from" << m_currentState << "to" << newState;

        if (newState == BlobConfig::IDLE &&
            (m_currentState == BlobConfig::MOVING || m_currentState == BlobConfig::RESIZING)) {

            qDebug() << "Przygotowanie płynnego przejścia do stanu IDLE";

            m_originalControlPoints = m_controlPoints;
            m_originalVelocities = m_velocity;
            m_originalBlobCenter = m_blobCenter;

            QPointF targetCenter = QPointF(width() / 2.0, height() / 2.0);
            std::vector<QPointF> targetPoints(m_controlPoints.size());

            for (size_t i = 0; i < m_controlPoints.size(); ++i) {
                QPointF relativePos = m_controlPoints[i] - m_blobCenter;
                targetPoints[i] = targetCenter + relativePos;
            }

            m_targetIdlePoints = targetPoints;
            m_targetIdleCenter = targetCenter;

            m_inTransitionToIdle = true;
            m_transitionToIdleStartTime = QDateTime::currentMSecsSinceEpoch();
            m_transitionToIdleDuration = 1000;

            if (m_transitionToIdleTimer) {
                m_transitionToIdleTimer->stop();
            }


            m_currentState = BlobConfig::IDLE;

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
    update();
}

void BlobAnimation::setGridColor(const QColor &color) {
    m_params.gridColor = color;
    update();
}

void BlobAnimation::setGridSpacing(int spacing) {
    m_params.gridSpacing = spacing;
    update();
}

bool BlobAnimation::eventFilter(QObject *watched, QEvent *event) {

    if (watched == window()) {
        if (event->type() == QEvent::Move) {
            qDebug() << "WINDOW MOVE EVENT DETECTED via event filter!";

            QPointF currentWindowPos = window()->pos();
            QVector2D windowVelocity = m_physics.calculateWindowVelocity(currentWindowPos);

            qDebug() << "Window velocity:" << windowVelocity.x() << "," << windowVelocity.y()
                     << "length:" << windowVelocity.length();

            if (windowVelocity.length() > 0.2) {
                qDebug() << "Applying inertia force via event filter!";

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
    }

    return QWidget::eventFilter(watched, event);
}