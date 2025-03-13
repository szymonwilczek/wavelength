#include "blob_transition_manager.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

BlobTransitionManager::BlobTransitionManager(QObject* parent)
    : QObject(parent)
{
}

BlobTransitionManager::~BlobTransitionManager()
{
    if (m_transitionToIdleTimer) {
        m_transitionToIdleTimer->stop();
        delete m_transitionToIdleTimer;
        m_transitionToIdleTimer = nullptr;
    }
}

void BlobTransitionManager::startTransitionToIdle(
    const std::vector<QPointF>& currentControlPoints, 
    const std::vector<QPointF>& currentVelocity,
    const QPointF& currentBlobCenter,
    const QPointF& targetCenter,
    int width, 
    int height)
{
    m_originalControlPoints = currentControlPoints;
    m_originalVelocities = currentVelocity;
    m_originalBlobCenter = currentBlobCenter;

    QPointF targetIdleCenter = targetCenter.isNull() ? QPointF(width / 2.0, height / 2.0) : targetCenter;

    double currentRadius = 0.0;
    for (const auto& point : currentControlPoints) {
        currentRadius += QVector2D(point - currentBlobCenter).length();
    }
    currentRadius /= currentControlPoints.size();

    // Obliczamy stosunek aktualnego promienia do docelowego
    double targetRadius = 250.0f; // Standardowy promień bloba
    double radiusRatio = currentRadius / targetRadius;

    std::vector<QPointF> targetPoints(currentControlPoints.size());

    // Przygotowujemy docelowe punkty dla stanu IDLE
    for (size_t i = 0; i < currentControlPoints.size(); ++i) {
        QPointF relativePos = currentControlPoints[i] - currentBlobCenter;
        if (radiusRatio > 0.1) {
            relativePos = relativePos / radiusRatio;
        }
        targetPoints[i] = targetIdleCenter + relativePos;
    }

    m_targetIdlePoints = targetPoints;
    m_targetIdleCenter = targetIdleCenter;

    m_inTransitionToIdle = true;
    m_transitionToIdleStartTime = QDateTime::currentMSecsSinceEpoch();
    m_transitionToIdleDuration = 700; // 700 ms trwania animacji

    if (m_transitionToIdleTimer) {
        m_transitionToIdleTimer->stop();
        delete m_transitionToIdleTimer;
    }

    m_transitionToIdleTimer = nullptr;
}

void BlobTransitionManager::handleIdleTransition(
    std::vector<QPointF>& controlPoints,
    std::vector<QPointF>& velocity,
    QPointF& blobCenter,
    const BlobConfig::BlobParameters& params,
    std::function<void(std::vector<QPointF>&, std::vector<QPointF>&, QPointF&, const BlobConfig::BlobParameters&)> applyIdleState)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsedMs = currentTime - m_transitionToIdleStartTime;

    if (elapsedMs >= m_transitionToIdleDuration) {
        // Zakończenie animacji przejścia
        m_inTransitionToIdle = false;

        // Wygaszanie prędkości
        std::for_each(velocity.begin(), velocity.end(), [](QPointF& vel) {
            vel *= 0.7;
        });

        // Stosujemy efekt stanu IDLE
        applyIdleState(controlPoints, velocity, blobCenter, params);

        emit transitionCompleted();
    } else {
        double progress = elapsedMs / (double)m_transitionToIdleDuration;

        // Wygładzenie animacji (efekt ease-in-out)
        double easedProgress = progress;
        if (progress < 0.3) {
            easedProgress = progress * (0.7 + progress * 0.3);
        } else if (progress > 0.7) {
            double t = 1.0 - progress;
            easedProgress = 1.0 - t * (0.7 + t * 0.3);
        }

        // Płynna zmiana pozycji centrum bloba
        blobCenter = QPointF(
            m_originalBlobCenter.x() * (1.0 - easedProgress) + m_targetIdleCenter.x() * easedProgress,
            m_originalBlobCenter.y() * (1.0 - easedProgress) + m_targetIdleCenter.y() * easedProgress
        );

        // Płynna zmiana pozycji punktów kontrolnych
        std::transform(
            m_originalControlPoints.begin(), m_originalControlPoints.end(),
            m_targetIdlePoints.begin(),
            controlPoints.begin(),
            [easedProgress](const QPointF& orig, const QPointF& target) {
                return QPointF(
                    orig.x() * (1.0 - easedProgress) + target.x() * easedProgress,
                    orig.y() * (1.0 - easedProgress) + target.y() * easedProgress
                );
            }
        );

        // Stopniowe wygaszanie prędkości
        double dampingFactor = pow(0.98, 1.0 + 3.0 * easedProgress);
        for (size_t i = 0; i < velocity.size(); ++i) {
            velocity[i] *= dampingFactor;
        }

        // Stopniowe wprowadzanie efektu stanu IDLE
        double idleBlendFactor = easedProgress * 0.4;

        std::vector<QPointF> tempVelocity = velocity;
        QPointF tempCenter = blobCenter;
        std::vector<QPointF> tempPoints = controlPoints;

        applyIdleState(tempPoints, tempVelocity, tempCenter, params);

        for (size_t i = 0; i < velocity.size(); ++i) {
            velocity[i] += (tempVelocity[i] - velocity[i]) * idleBlendFactor;
        }
    }
}

void BlobTransitionManager::processMovementBuffer(
    std::vector<QPointF>& velocity,
    QPointF& blobCenter,
    std::vector<QPointF>& controlPoints,
    float blobRadius,
    std::function<void(std::vector<QPointF>&, QPointF&, std::vector<QPointF>&, float, QVector2D)> applyInertiaForce,
    std::function<void(const QPointF&)> setLastWindowPos)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (m_movementBuffer.size() < 2)
        return;

    QVector2D newVelocity(0, 0);
    double totalWeight = 0;

    // Obliczanie wektora prędkości na podstawie próbek ruchu
    for (size_t i = 1; i < m_movementBuffer.size(); i++) {
        QPointF posDelta = m_movementBuffer[i].position - m_movementBuffer[i-1].position;
        qint64 timeDelta = m_movementBuffer[i].timestamp - m_movementBuffer[i-1].timestamp;

        if (timeDelta > 0) {
            double scale = 700.0 / timeDelta;
            QVector2D sampleVelocity(posDelta.x() * scale, posDelta.y() * scale);

            // Nowsze próbki mają większą wagę
            double weight = 0.5 + 0.5 * (i / (double)(m_movementBuffer.size() - 1));

            newVelocity += sampleVelocity * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight > 0) {
        newVelocity /= totalWeight;
    }

    // Wygładzanie prędkości
    if (m_smoothedVelocity.length() > 0) {
        m_smoothedVelocity = m_smoothedVelocity * 0.7 + newVelocity * 0.2;
    } else {
        m_smoothedVelocity = newVelocity;
    }

    double velocityMagnitude = m_smoothedVelocity.length();
    bool significantMovement = velocityMagnitude > 0.3;

    if (significantMovement || (currentTime - m_lastMovementTime) < 200) { // 200ms "pamięci" ruchu
        if (significantMovement) {
            // Stosujemy siłę inercji do blobu
            QVector2D scaledVelocity = m_smoothedVelocity * 0.6;
            applyInertiaForce(
                velocity,
                blobCenter,
                controlPoints,
                blobRadius,
                scaledVelocity
            );

            // Aktualizujemy pozycję okna
            setLastWindowPos(m_movementBuffer.back().position);
            
            m_isMoving = true;
            m_inactivityCounter = 0;
            
            emit significantMovementDetected();
        }
    } else if (m_isMoving) {
        m_inactivityCounter++;

        if (m_inactivityCounter > 60) { // około 1s nieaktywności przy 60 FPS
            m_isMoving = false;
            emit movementStopped();
        }
    }

    // Czyścimy bufor ruchu po okresie bezczynności
    if (m_movementBuffer.size() > 1 && (currentTime - m_lastMovementTime) > 500) { // 0.5s bez ruchu
        QPointF lastPosition = m_movementBuffer.back().position;
        qint64 lastTimestamp = m_movementBuffer.back().timestamp;

        m_movementBuffer.clear();
        m_movementBuffer.push_back({lastPosition, lastTimestamp});
    }
}

void BlobTransitionManager::addMovementSample(const QPointF& position, qint64 timestamp)
{
    m_movementBuffer.push_back({position, timestamp});
    if (m_movementBuffer.size() > MAX_MOVEMENT_SAMPLES) {
        m_movementBuffer.pop_front();
    }
    m_lastMovementTime = timestamp;
}

void BlobTransitionManager::clearMovementBuffer()
{
    m_movementBuffer.clear();
}