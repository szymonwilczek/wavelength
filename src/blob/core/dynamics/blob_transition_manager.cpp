#include "blob_transition_manager.h"
#include <QDebug>
#include <qmath.h>

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


void BlobTransitionManager::processMovementBuffer(
    std::vector<QPointF>& velocity,
    QPointF& blobCenter,
    std::vector<QPointF>& controlPoints,
    const float blobRadius,
    const std::function<void(std::vector<QPointF>&, QPointF&, std::vector<QPointF>&, float, QVector2D)> &applyInertiaForce,
    const std::function<void(const QPointF&)> &setLastWindowPos)
{
    if (m_isResizing) {
        return;
    }

    const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (m_movementBuffer.size() < 2)
        return;

    QVector2D newVelocity(0, 0);
    double totalWeight = 0;

    // Obliczanie wektora prędkości na podstawie próbek ruchu
    for (size_t i = 1; i < m_movementBuffer.size(); i++) {
        QPointF posDelta = m_movementBuffer[i].position - m_movementBuffer[i-1].position;

        if (const qint64 timeDelta = m_movementBuffer[i].timestamp - m_movementBuffer[i-1].timestamp; timeDelta > 0) {
            const double scale = 700.0 / timeDelta;
            QVector2D sampleVelocity(posDelta.x() * scale, posDelta.y() * scale);

            // Nowsze próbki mają większą wagę
            const double weight = 0.5 + 0.5 * (i / static_cast<double>(m_movementBuffer.size() - 1));

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

    const double velocityMagnitude = m_smoothedVelocity.length();

    if (const bool significantMovement = velocityMagnitude > 0.3; significantMovement || (currentTime - m_lastMovementTime) < 200) { // 200ms "pamięci" ruchu
        if (significantMovement) {
            // Stosujemy siłę inercji do blobu
            const QVector2D scaledVelocity = m_smoothedVelocity * 0.6;
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
        const QPointF lastPosition = m_movementBuffer.back().position;
        const qint64 lastTimestamp = m_movementBuffer.back().timestamp;

        m_movementBuffer.clear();
        m_movementBuffer.push_back({lastPosition, lastTimestamp});
    }
}

void BlobTransitionManager::addMovementSample(const QPointF& position, const qint64 timestamp)
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