#include "blob_animation.h"

#include <QApplication>
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

    // Konfiguracja timera sprawdzającego pozycję okna - wysoka częstotliwość
    m_windowPositionTimer.setInterval(8); // Sprawdzaj jeszcze częściej (>100 FPS)
    m_windowPositionTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_windowPositionTimer, &QTimer::timeout, this, &BlobAnimation::checkWindowPosition);
    m_windowPositionTimer.start();

    connect(&m_stateResetTimer, &QTimer::timeout, this, &BlobAnimation::onStateResetTimeout);

    m_eventReEnableTimer.setSingleShot(true);
    connect(&m_eventReEnableTimer, &QTimer::timeout, this, [this]() {
        m_eventsEnabled = true;
        qDebug() << "Events re-enabled";
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
    if (!currentWindow || !m_eventsEnabled || m_inTransitionToIdle)
        return;

    // Ta metoda jest wywoływana przez timer, ale nie potrzebujemy sprawdzać pozycji
    // za każdym razem - eventy są już filtrowane w eventFilter
    // i zapisywane do bufora m_movementBuffer

    // Sprawdzamy tylko czy nie ma bezczynności
    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

    if (!m_movementBuffer.empty() &&
        (currentTimestamp - m_movementBuffer.back().timestamp) > 100) {
        // Dodaj potwierdzenie statusu nieruchomego po okresie bezczynności
        QPointF currentWindowPos = currentWindow->pos();

        // Dodaj próbkę z obecną pozycją tylko jeśli się zmieniła
        if (m_movementBuffer.empty() || currentWindowPos != m_movementBuffer.back().position) {
            m_movementBuffer.push_back({currentWindowPos, currentTimestamp});

            // Ogranicz rozmiar bufora
            if (m_movementBuffer.size() > MAX_MOVEMENT_SAMPLES) {
                m_movementBuffer.pop_front();
            }
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

    if (m_currentState == BlobConfig::IDLE && !m_isBeingAbsorbed && !m_isAbsorbing) {
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

    if (m_isBeingAbsorbed || m_isClosingAfterAbsorption) {
        painter.setOpacity(m_absorptionOpacity);
        painter.save();
        QPointF center = getBlobCenter();
        painter.translate(center);
        painter.scale(m_absorptionScale, m_absorptionScale);
        painter.translate(-center);
    }
    else if (m_isAbsorbing) {
        painter.save();
        QPointF center = getBlobCenter();

        if (m_pulseAnimation && m_absorptionPulse > 0.0) {
            QPen extraGlowPen(m_params.borderColor.lighter(150));
            extraGlowPen.setWidth(m_params.borderWidth + 10);
            painter.setPen(extraGlowPen);

            QPainterPath blobPath = BlobPath::createBlobPath(m_controlPoints, m_controlPoints.size());
            painter.drawPath(blobPath);
        }

        painter.translate(center);
        painter.scale(m_absorptionScale, m_absorptionScale);
        painter.translate(-center);
    }

    m_renderer.renderBlob(painter, m_controlPoints, m_blobCenter,
                             m_params, width(), height());

    if (m_isBeingAbsorbed || m_isAbsorbing) {
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
    qDebug() << "BlobAnimation: Rozpoczęcie procesu bycia absorbowanym";
    m_isBeingAbsorbed = true;
    m_absorptionOpacity = 1.0f;
    m_absorptionScale = 1.0f;

    if (m_currentState == BlobConfig::IDLE) {
        m_animationTimer.stop();
    }

    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
    }

    if (m_opacityAnimation) {
        m_opacityAnimation->stop();
        delete m_opacityAnimation;
    }

    m_scaleAnimation = new QPropertyAnimation(this, "absorptionScale");
    m_scaleAnimation->setDuration(8000);  // 2 sekundy
    m_scaleAnimation->setStartValue(1.0);
    m_scaleAnimation->setEndValue(0.1);
    m_scaleAnimation->setEasingCurve(QEasingCurve::InQuad);

    m_opacityAnimation = new QPropertyAnimation(this, "absorptionOpacity");
    m_opacityAnimation->setDuration(8000);  // 2 sekundy
    m_opacityAnimation->setStartValue(1.0);
    m_opacityAnimation->setEndValue(0.0);
    m_opacityAnimation->setEasingCurve(QEasingCurve::InQuad);

    m_scaleAnimation->start();
    m_opacityAnimation->start();

    update();
}

void BlobAnimation::finishBeingAbsorbed() {
    qDebug() << "BlobAnimation: Zakończenie procesu bycia absorbowanym";
    m_isClosingAfterAbsorption = true;

    m_absorptionOpacity = 0.0f;
    m_absorptionScale = 0.1f;
    update();

    QTimer::singleShot(500, []() {
        qDebug() << "Aplikacja zostanie zamknięta po absorpcji";
        QApplication::quit();
    });
}

void BlobAnimation::cancelAbsorption() {
    qDebug() << "BlobAnimation: Anulowanie procesu bycia absorbowanym";
    m_isBeingAbsorbed = false;

    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
        m_scaleAnimation = nullptr;
    }

    if (m_opacityAnimation) {
        m_opacityAnimation->stop();
        delete m_opacityAnimation;
        m_opacityAnimation = nullptr;
    }

    m_scaleAnimation = new QPropertyAnimation(this, "absorptionScale");
    m_scaleAnimation->setDuration(500);  // 0.5 sekundy
    m_scaleAnimation->setStartValue(m_absorptionScale);
    m_scaleAnimation->setEndValue(1.0);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad);

    m_opacityAnimation = new QPropertyAnimation(this, "absorptionOpacity");
    m_opacityAnimation->setDuration(500);  // 0.5 sekundy
    m_opacityAnimation->setStartValue(m_absorptionOpacity);
    m_opacityAnimation->setEndValue(1.0);
    m_opacityAnimation->setEasingCurve(QEasingCurve::OutQuad);

    m_scaleAnimation->start();
    m_opacityAnimation->start();

    if (m_currentState == BlobConfig::IDLE) {
        QTimer::singleShot(500, [this]() {
            if (!m_isBeingAbsorbed && !m_isAbsorbing) {
                m_animationTimer.start();
            }
        });
    }

    update();
}

void BlobAnimation::startAbsorbing(const QString& targetId) {
    qDebug() << "BlobAnimation: Rozpoczęcie absorbowania innej instancji:" << targetId;
    m_isAbsorbing = true;
    m_absorptionTargetId = targetId;
    m_absorptionScale = 1.0f;
    m_absorptionOpacity = 1.0f;

    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
    }

    m_scaleAnimation = new QPropertyAnimation(this, "absorptionScale");
    m_scaleAnimation->setDuration(8000);  // 8 sekund
    m_scaleAnimation->setStartValue(1.0);
    m_scaleAnimation->setEndValue(1.8);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad);
    m_scaleAnimation->start();

    if (m_pulseAnimation) {
        m_pulseAnimation->stop();
        delete m_pulseAnimation;
    }

    m_pulseAnimation = new QPropertyAnimation(this, "absorptionPulse");
    m_pulseAnimation->setDuration(800);  // krótszy czas = szybsze pulsowanie
    m_pulseAnimation->setStartValue(0.0);
    m_pulseAnimation->setEndValue(1.0);
    m_pulseAnimation->setLoopCount(-1);  // pętla nieskończona
    m_pulseAnimation->start();

    update();
}

void BlobAnimation::finishAbsorbing(const QString& targetId) {
    qDebug() << "BlobAnimation: Zakończenie absorbowania instancji:" << targetId;
    m_isAbsorbing = false;

    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
        m_scaleAnimation = nullptr;
    }

    m_scaleAnimation = new QPropertyAnimation(this, "absorptionScale");
    m_scaleAnimation->setDuration(500);  // 0.5 sekundy
    m_scaleAnimation->setStartValue(m_absorptionScale);
    m_scaleAnimation->setEndValue(1.0);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad);
    m_scaleAnimation->start();

    m_absorptionTargetId.clear();
    update();
}

void BlobAnimation::cancelAbsorbing(const QString& targetId) {
    qDebug() << "BlobAnimation: Anulowanie absorbowania instancji:" << targetId;
    m_isAbsorbing = false;

    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
        m_scaleAnimation = nullptr;
    }

    m_scaleAnimation = new QPropertyAnimation(this, "absorptionScale");
    m_scaleAnimation->setDuration(500);  // 0.5 sekundy
    m_scaleAnimation->setStartValue(m_absorptionScale);
    m_scaleAnimation->setEndValue(1.0);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad);
    m_scaleAnimation->start();

    m_absorptionTargetId.clear();
    update();
}

void BlobAnimation::updateAbsorptionProgress(float progress) {
    if (m_isBeingAbsorbed) {
        m_absorptionScale = 1.0f - progress * 0.9f;
        m_absorptionOpacity = 1.0f - progress;
        qDebug() << "Blob pochłaniany: skala=" << m_absorptionScale << "przezroczystość=" << m_absorptionOpacity;
    }
    else if (m_isAbsorbing) {
        m_absorptionScale = 1.0f + progress * 0.3f;
        qDebug() << "Blob pochłaniający: skala=" << m_absorptionScale;
    }

    update();
}

void BlobAnimation::updateAnimation() {
    m_needsRedraw = false;

    // Przetwarzaj bufor ruchów przy każdej klatce animacji (60 FPS)
    processMovementBuffer();

    if (m_inTransitionToIdle) {
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

// Nowa metoda do przetwarzania bufora ruchów
void BlobAnimation::processMovementBuffer() {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // Jeśli bufor jest pusty lub nie zawiera wystarczającej liczby próbek, zakończ
    if (m_movementBuffer.size() < 2)
        return;

    // Oblicz wygładzoną prędkość na podstawie bufora próbek
    QVector2D newVelocity(0, 0);
    double totalWeight = 0;

    // Przetwarzamy wszystkie próbki, nadając większą wagę nowszym próbkom
    for (size_t i = 1; i < m_movementBuffer.size(); i++) {
        QPointF posDelta = m_movementBuffer[i].position - m_movementBuffer[i-1].position;
        qint64 timeDelta = m_movementBuffer[i].timestamp - m_movementBuffer[i-1].timestamp;

        if (timeDelta > 0) {
            // Oblicz wektor prędkości dla danej pary próbek
            double scale = 700.0 / timeDelta; // Skalowanie do jednostek/s
            QVector2D sampleVelocity(posDelta.x() * scale, posDelta.y() * scale);

            // Waga próbki - nowsze próbki mają większą wagę
            double weight = 0.5 + 0.5 * (i / (double)(m_movementBuffer.size() - 1));

            // Dodaj do wygładzonej prędkości
            newVelocity += sampleVelocity * weight;
            totalWeight += weight;
        }
    }

    // Normalizacja prędkości
    if (totalWeight > 0) {
        newVelocity /= totalWeight;
    }

    // Wygładź prędkość z poprzednią wartością dla bardziej płynnych przejść
    if (m_smoothedVelocity.length() > 0) {
        m_smoothedVelocity = m_smoothedVelocity * 0.7 + newVelocity * 0.2;
    } else {
        m_smoothedVelocity = newVelocity;
    }

    double velocityMagnitude = m_smoothedVelocity.length();

    // Sprawdź czy wykryto znaczący ruch
    bool significantMovement = velocityMagnitude > 0.3;

    // Jeśli wykryto znaczący ruch lub niedawno był ruch
    if (significantMovement || (currentTime - m_lastMovementTime) < 200) { // 200ms "pamięci" ruchu
        if (significantMovement) {
            // Przejdź do stanu ruchu i zastosuj efekty fizyczne
            switchToState(BlobConfig::MOVING);

            QVector2D scaledVelocity = m_smoothedVelocity * 0.6;
            m_movingState->applyInertiaForce(
                m_velocity,
                m_blobCenter,
                m_controlPoints,
                m_params.blobRadius,
                scaledVelocity
            );

            m_physics.setLastWindowPos(m_movementBuffer.back().position);
            m_isMoving = true;
            m_inactivityCounter = 0;
            m_needsRedraw = true;
        }
    } else if (m_isMoving) {
        // Zliczanie nieaktywności i ewentualne przejście do stanu bezczynności
        m_inactivityCounter++;

        if (m_inactivityCounter > 60) { // Po około 1s nieaktywności przy 60 FPS
            m_isMoving = false;
            if (m_currentState == BlobConfig::MOVING) {
                switchToState(BlobConfig::IDLE);
            }
        }
    }

    // Wyczyść stare próbki, pozostawiając tylko ostatnią jako punkt odniesienia dla nowych
    if (m_movementBuffer.size() > 1 && (currentTime - m_lastMovementTime) > 500) { // 0.5s bez ruchu
        QPointF lastPosition = m_movementBuffer.back().position;
        qint64 lastTimestamp = m_movementBuffer.back().timestamp;

        m_movementBuffer.clear();
        m_movementBuffer.push_back({lastPosition, lastTimestamp});
    }
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
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsedMs = currentTime - m_transitionToIdleStartTime;

        if (elapsedMs >= m_transitionToIdleDuration) {

            m_inTransitionToIdle = false;
            m_currentBlobState = m_idleState.get();

            std::for_each(m_velocity.begin(), m_velocity.end(), [](QPointF& vel) {
            vel *= 0.7;
            });

            m_idleState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
            m_eventReEnableTimer.start(200);

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

            std::transform(
            m_originalControlPoints.begin(), m_originalControlPoints.end(),
            m_targetIdlePoints.begin(),
            m_controlPoints.begin(),
            [easedProgress](const QPointF& orig, const QPointF& target) {
                return QPointF(
                    orig.x() * (1.0 - easedProgress) + target.x() * easedProgress,
                    orig.y() * (1.0 - easedProgress) + target.y() * easedProgress
                );
            }
        );

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
        }
}


void BlobAnimation::resizeEvent(QResizeEvent *event) {
    if (!m_eventsEnabled || m_inTransitionToIdle) {
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

    if (!m_eventsEnabled || m_inTransitionToIdle) {
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

    if (!m_eventsEnabled || m_inTransitionToIdle) {
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
    if (!m_eventsEnabled || m_inTransitionToIdle) {
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

                    m_movementBuffer.push_back({currentWindowPos, currentTime});
                    if (m_movementBuffer.size() > MAX_MOVEMENT_SAMPLES) {
                        m_movementBuffer.pop_front();
                    }
                    m_lastMovementTime = currentTime;
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