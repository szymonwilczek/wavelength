#include "blob_animation.h"
#include <QPainter>
#include <QDebug>

BlobAnimation::BlobAnimation(QWidget *parent)
    : QWidget(parent)
{
    m_params.blobRadius = 200.0f;
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

    bool shouldLog = (frameCounter % 30 == 0);

    if (m_currentState == BlobConfig::IDLE) {
        if (shouldLog) qDebug() << "Animation update: IDLE state";
        applyIdleEffect();
    }
    else if (m_currentState == BlobConfig::MOVING) {
        if (shouldLog) qDebug() << "Animation update: MOVING state";

        if (window()) {
            QVector2D windowVelocity = m_physics.getLastWindowVelocity();
            if (shouldLog) qDebug() << "Current window velocity:" << windowVelocity.x() << "," << windowVelocity.y();

            if (windowVelocity.length() > 0.1) {
                if (shouldLog) qDebug() << "Applying moving state effect";
                m_movingState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
            }
        }
    } else if (m_currentState == BlobConfig::RESIZING) {
        if (shouldLog) qDebug() << "Animation update: RESIZING state";
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

            qDebug() << "Starting transition to IDLE state";

            for (auto& vel : m_velocity) {
                vel *= 0.7;
            }

            if (!m_transitionToIdleTimer) {
                m_transitionToIdleTimer = new QTimer(this);
                m_transitionToIdleTimer->setTimerType(Qt::PreciseTimer);
                connect(m_transitionToIdleTimer, &QTimer::timeout, this, [this]() {
                    static int transitionSteps = 0;
                    const int totalSteps = 10;

                    transitionSteps++;

                    if (transitionSteps >= totalSteps) {
                        m_transitionToIdleTimer->stop();
                        transitionSteps = 0;

                        m_physics.smoothBlobShape(m_controlPoints);

                        qDebug() << "Transition to IDLE completed";
                    } else {
                        double transitionProgress = static_cast<double>(transitionSteps) / totalSteps;
                        double stabilizationRate = 0.02 + transitionProgress * 0.1;

                        for (auto& vel : m_velocity) {
                            vel *= (1.0 - transitionProgress * 0.3);
                        }

                        m_physics.stabilizeBlob(m_controlPoints, m_blobCenter,
                                             m_params.blobRadius, stabilizationRate);
                    }
                });
            }

            m_transitionToIdleTimer->start(50);
        }

        m_currentState = newState;

        // Aktualizacja wskaźnika na aktualny stan
        switch (newState) {
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

        // Resetowanie timera powrotu do stanu bezczynności
        if (newState != BlobConfig::IDLE) {
            m_stateResetTimer.stop();
        }

        if (newState == BlobConfig::MOVING || newState == BlobConfig::RESIZING) {
            m_stateResetTimer.start(1000);
        }
    }
}

void BlobAnimation::applyForces(const QVector2D &force) {
    // Deleguj aplikowanie sił do aktualnego stanu
    m_currentBlobState->applyForce(force, m_velocity, m_blobCenter,
                                 m_controlPoints, m_params.blobRadius);
}

void BlobAnimation::applyIdleEffect() {
    // Deleguj obsługę stanu bezczynności do IdleState
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
    // Sprawdź czy zdarzenie dotyczy okna
    if (watched == window()) {
        if (event->type() == QEvent::Move) {
            qDebug() << "WINDOW MOVE EVENT DETECTED via event filter!";

            QPointF currentWindowPos = window()->pos();
            QVector2D windowVelocity = m_physics.calculateWindowVelocity(currentWindowPos);

            qDebug() << "Window velocity:" << windowVelocity.x() << "," << windowVelocity.y()
                     << "length:" << windowVelocity.length();

            // Zwiększony próg wykrycia ruchu
            if (windowVelocity.length() > 0.2) {
                qDebug() << "Applying inertia force via event filter!";

                switchToState(BlobConfig::MOVING);

                // Bezpośrednie wywołanie stanu ruchomego
                m_movingState->applyInertiaForce(m_velocity, m_blobCenter, m_controlPoints,
                                             m_params.blobRadius, windowVelocity);

                // Bezpośrednie zastosowanie siły do punktów kontrolnych
                for (size_t i = 0; i < m_controlPoints.size(); ++i) {
                    double randX = (qrand() % 100 - 50) / 500.0;
                    double randY = (qrand() % 100 - 50) / 500.0;
                    QPointF randomVariation(randX, randY);

                    QPointF force(-windowVelocity.x() * 0.002, -windowVelocity.y() * 0.002);
                    force += randomVariation;
                    m_velocity[i] += force;
                }

                // Reset timera resetowania stanu
                m_stateResetTimer.stop();
                m_stateResetTimer.start(1000);
            }

            m_physics.setLastWindowPos(currentWindowPos);

            // Wymuszenie aktualizacji
            updateAnimation();
        }
    }

    // Zawsze przekazuj zdarzenie do standardowej obsługi
    return QWidget::eventFilter(watched, event);
}