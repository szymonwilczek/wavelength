#include "blob_animation.h"

#include <QApplication>
#include <QPainter>
#include <QDebug>
#include <QElapsedTimer>

BlobAnimation::BlobAnimation(QWidget *parent)
    : QWidget(parent),
    m_absorption(this),
    m_transitionManager(this)
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

    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, true);

    initializeBlob();

    m_resizeDebounceTimer.setSingleShot(true);
    m_resizeDebounceTimer.setInterval(50); // 50 ms debounce
    connect(&m_resizeDebounceTimer, &QTimer::timeout, this, &BlobAnimation::handleResizeTimeout);

    if (window()) {
        m_lastWindowPos = window()->pos();
        m_lastWindowPosForTimer = m_lastWindowPos;
        window()->installEventFilter(this);
    }

    m_animationTimer.setTimerType(Qt::PreciseTimer);
    m_animationTimer.setInterval(16); // ~60 FPS
    connect(&m_animationTimer, &QTimer::timeout, this, &BlobAnimation::updateAnimation);
    m_animationTimer.start();

    m_windowPositionTimer.setInterval(8); // (>100 FPS)
    m_windowPositionTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_windowPositionTimer, &QTimer::timeout, this, &BlobAnimation::checkWindowPosition);
    m_windowPositionTimer.start();

    connect(&m_stateResetTimer, &QTimer::timeout, this, &BlobAnimation::onStateResetTimeout);

    m_eventReEnableTimer.setSingleShot(true);
    connect(&m_eventReEnableTimer, &QTimer::timeout, this, [this]() {
        m_eventsEnabled = true;
        qDebug() << "Events re-enabled";
    });

    connect(&m_absorption, &BlobAbsorption::redrawNeeded, this, [this]() {
        update();
    });

    connect(&m_absorption, &BlobAbsorption::absorptionFinished, this, [this] {});

    connect(&m_transitionManager, &BlobTransitionManager::transitionCompleted, this, [this]() {
        m_eventReEnableTimer.start(200);
    });

    connect(&m_transitionManager, &BlobTransitionManager::significantMovementDetected, this, [this]() {
        switchToState(BlobConfig::MOVING);
        m_needsRedraw = true;
    });

    connect(&m_transitionManager, &BlobTransitionManager::movementStopped, this, [this]() {
        if (m_currentState == BlobConfig::MOVING) {
            switchToState(BlobConfig::IDLE);
        }
    });
}

void BlobAnimation::handleResizeTimeout() {
    QSize currentSize = size();
    if (currentSize != m_lastSize) {
        switchToState(BlobConfig::RESIZING);
        m_resizingState->handleResize(m_controlPoints, m_targetPoints, m_velocity,
                                    m_blobCenter, m_lastSize, currentSize);
        m_renderer.resetGridBuffer();
        m_lastSize = currentSize;

        m_stateResetTimer.stop();
        m_stateResetTimer.start(2000);
        update();
    }
}

void BlobAnimation::checkWindowPosition() {
    QWidget* currentWindow = window();
    if (!currentWindow || !m_eventsEnabled || m_transitionManager.isInTransitionToIdle())
        return;

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

    std::pmr::deque<BlobTransitionManager::WindowMovementSample> movementBuffer = m_transitionManager.getMovementBuffer();

    // Sprawdzenie ostatniego czasu ruchu
    if (movementBuffer.empty() ||
        (currentTimestamp - m_transitionManager.getLastMovementTime()) > 100) {
        QPointF currentWindowPos = currentWindow->pos();

        if (movementBuffer.empty() || currentWindowPos != movementBuffer.back().position) {
            m_transitionManager.addMovementSample(currentWindowPos, currentTimestamp);
        }
        }
}

BlobAnimation::~BlobAnimation() {
    m_animationTimer.stop();
    m_idleTimer.stop();
    m_stateResetTimer.stop();

    m_idleState.reset();
    m_movingState.reset();
    m_resizingState.reset();

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

    static QPixmap backgroundCache;
    static QSize lastBackgroundSize;
    static QColor lastBgColor;
    static QColor lastGridColor;
    static int lastGridSpacing;

    bool shouldUpdateBackgroundCache =
        backgroundCache.isNull() ||
        lastBackgroundSize != size() ||
        lastBgColor != m_params.backgroundColor ||
        lastGridColor != m_params.gridColor ||
        lastGridSpacing != m_params.gridSpacing;

    QPainter painter(this);

    if (m_currentState == BlobConfig::MOVING || m_currentState == BlobConfig::RESIZING) {
        painter.setRenderHint(QPainter::Antialiasing, false);
    } else {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    }

    if (m_currentState == BlobConfig::IDLE && !m_absorption.isBeingAbsorbed() && !m_absorption.isAbsorbing()) {
        if (shouldUpdateBackgroundCache) {
            backgroundCache = QPixmap(size());
            backgroundCache.fill(Qt::transparent);

            QPainter cachePainter(&backgroundCache);
            cachePainter.setRenderHint(QPainter::Antialiasing, true);
            m_renderer.drawBackground(cachePainter, m_params.backgroundColor,
                                   m_params.gridColor, m_params.gridSpacing,
                                   width(), height());

            lastBackgroundSize = size();
            lastBgColor = m_params.backgroundColor;
            lastGridColor = m_params.gridColor;
            lastGridSpacing = m_params.gridSpacing;
        }

        painter.drawPixmap(0, 0, backgroundCache);
    } else {
        painter.setRenderHint(QPainter::Antialiasing, m_currentState == BlobConfig::IDLE);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, m_currentState == BlobConfig::IDLE);

        m_renderer.drawBackground(painter, m_params.backgroundColor,
                               m_params.gridColor, m_params.gridSpacing,
                               width(), height());
    }

    if (m_absorption.isBeingAbsorbed() || m_absorption.isClosingAfterAbsorption()) {
        painter.setOpacity(m_absorption.opacity());
        painter.save();
        QPointF center = getBlobCenter();
        painter.translate(center);
        painter.scale(m_absorption.scale(), m_absorption.scale());
        painter.translate(-center);
    }
    else if (m_absorption.isAbsorbing()) {
        painter.save();
        QPointF center = getBlobCenter();

        if (m_absorption.isPulseActive()) {
            QPen extraGlowPen(m_params.borderColor.lighter(150));
            extraGlowPen.setWidth(m_params.borderWidth + 10);
            painter.setPen(extraGlowPen);

            QPainterPath blobPath = BlobPath::createBlobPath(m_controlPoints, m_controlPoints.size());
            painter.drawPath(blobPath);
        }

        painter.translate(center);
        painter.scale(m_absorption.scale(), m_absorption.scale());
        painter.translate(-center);
    }

    m_renderer.renderBlob(painter, m_controlPoints, m_blobCenter,
                             m_params, width(), height());

    if (m_absorption.isBeingAbsorbed() || m_absorption.isAbsorbing()) {
        painter.restore();
    }
}

QRectF BlobAnimation::calculateBlobBoundingRect() {
    if (m_controlPoints.empty()) {
        return QRectF(0, 0, width(), height());
    }

    auto xComp = [](const QPointF& a, const QPointF& b) { return a.x() < b.x(); };
    auto yComp = [](const QPointF& a, const QPointF& b) { return a.y() < b.y(); };

    auto [minXIt, maxXIt] = std::minmax_element(m_controlPoints.begin(), m_controlPoints.end(), xComp);
    auto [minYIt, maxYIt] = std::minmax_element(m_controlPoints.begin(), m_controlPoints.end(), yComp);

    qreal minX = minXIt->x();
    qreal maxX = maxXIt->x();
    qreal minY = minYIt->y();
    qreal maxY = maxYIt->y();

    int margin = m_params.borderWidth + m_params.glowRadius + 5;
    return QRectF(minX - margin, minY - margin,
                 maxX - minX + 2*margin, maxY - minY + 2*margin);
}

void BlobAnimation::startBeingAbsorbed() {
    m_absorption.startBeingAbsorbed();

    if (m_currentState == BlobConfig::IDLE) {
        m_animationTimer.stop();
    }
}

void BlobAnimation::finishBeingAbsorbed() {
    m_absorption.finishBeingAbsorbed();
}

void BlobAnimation::cancelAbsorption() {
    m_absorption.cancelAbsorption();

    if (m_currentState == BlobConfig::IDLE && !m_absorption.isBeingAbsorbed() && !m_absorption.isAbsorbing()) {
        m_animationTimer.start();
    }
}

void BlobAnimation::startAbsorbing(const QString& targetId) {
    m_absorption.startAbsorbing(targetId);
}

void BlobAnimation::finishAbsorbing(const QString& targetId) {
    m_absorption.finishAbsorbing(targetId);
}

void BlobAnimation::cancelAbsorbing(const QString& targetId) {
    m_absorption.cancelAbsorbing(targetId);
}

void BlobAnimation::updateAbsorptionProgress(float progress) {
    m_absorption.updateAbsorptionProgress(progress);
}

void BlobAnimation::updateAnimation() {
    m_needsRedraw = false;

    processMovementBuffer();

    if (m_transitionManager.isInTransitionToIdle()) {
        handleIdleTransition();
        m_needsRedraw = true;
    } else if (m_currentState == BlobConfig::IDLE) {
        if (m_currentBlobState) {
            m_currentBlobState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
            m_needsRedraw = true;
        }
    } else if (m_currentState == BlobConfig::MOVING || m_currentState == BlobConfig::RESIZING) {
        if (m_currentBlobState) {
            m_currentBlobState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
            m_needsRedraw = true;
        }
    }

    updatePhysics();

    if (m_needsRedraw) {
        update();
    }
}

void BlobAnimation::processMovementBuffer() {
    m_transitionManager.processMovementBuffer(
        m_velocity,
        m_blobCenter,
        m_controlPoints,
        m_params.blobRadius,
        [this](std::vector<QPointF>& vel, QPointF& center, std::vector<QPointF>& points, float radius, QVector2D force) {
            m_movingState->applyInertiaForce(vel, center, points, radius, force);
        },
        [this](const QPointF& pos) {
            m_physics.setLastWindowPos(pos);
        }
    );
}

void BlobAnimation::updatePhysics() {
    m_physics.updatePhysicsParallel(m_controlPoints, m_targetPoints, m_velocity,
                          m_blobCenter, m_params, m_physicsParams);

    int padding = m_params.borderWidth + m_params.glowRadius;
    m_physics.handleBorderCollisions(m_controlPoints, m_velocity, m_blobCenter,
                                   width(), height(), m_physicsParams.restitution, padding);

    m_physics.smoothBlobShape(m_controlPoints);

    double minDistance = m_params.blobRadius * m_physicsParams.minNeighborDistance;
    double maxDistance = m_params.blobRadius * m_physicsParams.maxNeighborDistance;
    m_physics.constrainNeighborDistances(m_controlPoints, m_velocity, minDistance, maxDistance);
}

void BlobAnimation::handleIdleTransition() {
    m_transitionManager.handleIdleTransition(
        m_controlPoints,
        m_velocity,
        m_blobCenter,
        m_params,
        [this](std::vector<QPointF>& points, std::vector<QPointF>& vel, QPointF& center, const BlobConfig::BlobParameters& params) {
            m_idleState->apply(points, vel, center, params);
        }
    );
}


void BlobAnimation::resizeEvent(QResizeEvent *event) {
    if (!m_eventsEnabled || m_transitionManager.isInTransitionToIdle()) {
        QWidget::resizeEvent(event);
        return;
    }

    m_lastSize = event->oldSize();
    m_resizeDebounceTimer.start();

    QWidget::resizeEvent(event);
}

void BlobAnimation::onStateResetTimeout() {
    if (m_currentState != BlobConfig::IDLE) {
        qDebug() << "Auto switching to IDLE state due to inactivity";
        switchToState(BlobConfig::IDLE);
    }
}

bool BlobAnimation::event(QEvent *event) {

    if (!m_eventsEnabled || m_transitionManager.isInTransitionToIdle()) {
        return QWidget::event(event);
    }

    if (event->type() == QEvent::Resize) {
        static QSize lastSize = size();
        QSize currentSize = size();

        if (currentSize != lastSize) {
            switchToState(BlobConfig::RESIZING);
            lastSize = currentSize;

            m_stateResetTimer.stop();
            m_stateResetTimer.start(2000);
        }
    }

    return QWidget::event(event);
}

void BlobAnimation::switchToState(BlobConfig::AnimationState newState) {

    if (!m_eventsEnabled || m_transitionManager.isInTransitionToIdle()) {
        return;
    }


    if (m_currentState == newState) {
        if (newState == BlobConfig::MOVING || newState == BlobConfig::RESIZING) {
            m_stateResetTimer.stop();
            m_stateResetTimer.start(2000);
        }
        return;
    }

    qDebug() << "State change from" << m_currentState << "to" << newState;

    if (newState == BlobConfig::IDLE &&
    (m_currentState == BlobConfig::MOVING || m_currentState == BlobConfig::RESIZING)) {
        m_eventsEnabled = false;

        // Uruchom przejście do stanu IDLE przez transition manager
        m_transitionManager.startTransitionToIdle(
            m_controlPoints,
            m_velocity,
            m_blobCenter,
            QPointF(width() / 2.0, height() / 2.0),
            width(),
            height()
        );

        m_currentState = BlobConfig::IDLE;
        m_currentBlobState = m_idleState.get();

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
            m_stateResetTimer.stop();
            m_stateResetTimer.start(2000);
            break;
        case BlobConfig::RESIZING:
            m_currentBlobState = m_resizingState.get();
            m_stateResetTimer.stop();
            m_stateResetTimer.start(2000);
            break;
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
    if (!m_eventsEnabled || m_transitionManager.isInTransitionToIdle()) {
        return QWidget::eventFilter(watched, event);
    }

    static QPointF lastProcessedPos;
    static qint64 lastProcessedMoveTime = 0;

    if (watched == window()) {
        if (event->type() == QEvent::Move) {
            QMoveEvent *moveEvent = static_cast<QMoveEvent*>(event);
            QPointF newPos = moveEvent->pos();
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

            double moveDist = QVector2D(newPos - lastProcessedPos).length();
            qint64 timeDelta = currentTime - lastProcessedMoveTime;

            double distThreshold = 1.0;
            int timeThreshold = 5; // ms

            if (timeDelta < 8) {
                distThreshold = 2.0;
            } else if (timeDelta < 16) {
                distThreshold = 1.0;
            }

            if (moveDist > distThreshold || timeDelta > timeThreshold) {
                lastProcessedPos = newPos;
                lastProcessedMoveTime = currentTime;

                QWidget* currentWindow = window();
                if (currentWindow) {
                    QPointF currentWindowPos = currentWindow->pos();
                    m_lastWindowPosForTimer = currentWindowPos;

                    // Dodaj próbkę ruchu do transition managera
                    m_transitionManager.addMovementSample(currentWindowPos, currentTime);
                }
            } else {
                return true;
            }
        }

        else if (event->type() == QEvent::DragMove) {
            static qint64 lastDragEventTime = 0;
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

            if (currentTime - lastDragEventTime < 16) { // ~60 FPS
                return true;
            }

            lastDragEventTime = currentTime;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void BlobAnimation::setLifeColor(const QColor &color) {
    if (m_defaultLifeColor.isValid() == false) {
        m_defaultLifeColor = m_params.borderColor;
    }

    m_params.borderColor = color;

    m_needsRedraw = true;
    update();

    qDebug() << "Blob color changed to:" << color.name();
}

void BlobAnimation::resetLifeColor() {
    if (m_defaultLifeColor.isValid()) {
        m_params.borderColor = m_defaultLifeColor;

        m_needsRedraw = true;
        update();

        qDebug() << "Blob color reset to default:" << m_defaultLifeColor.name();
    }
}