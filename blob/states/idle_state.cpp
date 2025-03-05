#include "idle_state.h"
#include <cmath>
#include <qmath.h>

IdleState::IdleState() {}

void IdleState::updatePhases() {
    m_idleParams.wavePhase += 0.001;
    if (m_idleParams.wavePhase > 2 * M_PI) {
        m_idleParams.wavePhase -= 2 * M_PI;
    }

    // Aktualizacja drugiej fazy
    m_secondPhase += 0.002;
    if (m_secondPhase > 2 * M_PI) {
        m_secondPhase -= 2 * M_PI;
    }

    // Aktualizacja fazy rotacji
    m_rotationPhase += 0.0005;
    if (m_rotationPhase > 2 * M_PI) {
        m_rotationPhase -= 2 * M_PI;
    }
}

void IdleState::apply(std::vector<QPointF>& controlPoints,
                     std::vector<QPointF>& velocity,
                     QPointF& blobCenter,
                     const BlobConfig::BlobParameters& params) {

    updatePhases();

    double rotationStrength = 0.05 * std::sin(m_rotationPhase * 0.3);

    static std::vector<double> randomFactors;
    if (randomFactors.size() != controlPoints.size()) {
        randomFactors.resize(controlPoints.size());
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            randomFactors[i] = 0.8 + (qrand() % 40) / 100.0;
        }
    }

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        double angle = std::atan2(vectorFromCenter.y(), vectorFromCenter.x());
        double distanceFromCenter = QVector2D(vectorFromCenter).length();

        double waveStrength = m_idleParams.waveAmplitude * 0.25 * randomFactors[i] *
            std::sin(m_idleParams.wavePhase + m_idleParams.waveFrequency * angle);

        waveStrength += (m_idleParams.waveAmplitude * 0.15) * randomFactors[i] *
            std::sin(m_secondPhase + m_idleParams.waveFrequency * 1.5 * angle + 0.5);

        waveStrength += (m_idleParams.waveAmplitude * 0.1) * randomFactors[i] *
            std::sin(m_idleParams.wavePhase * 0.5) * std::cos(angle * 2);

        QVector2D normalizedVector = QVector2D(vectorFromCenter).normalized();

        QPointF perpVector(-normalizedVector.y(), normalizedVector.x());

        double rotationFactor = rotationStrength * (distanceFromCenter / params.blobRadius);
        QPointF rotationForce = rotationFactor * perpVector;

        QPointF forceVector = QPointF(normalizedVector.x(), normalizedVector.y()) * waveStrength + rotationForce;

        double forceScale = 0.3 + 0.7 * (distanceFromCenter / params.blobRadius);
        if (forceScale > 1.0) forceScale = 1.0;

        velocity[i] += forceVector * forceScale * 0.06;
    }
}

void IdleState::applyForce(const QVector2D& force,
                         std::vector<QPointF>& velocity,
                         QPointF& blobCenter,
                         const std::vector<QPointF>& controlPoints,
                         double blobRadius) {
    
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        double distanceFromCenter = QVector2D(vectorFromCenter).length();
        
        double forceScale = distanceFromCenter / blobRadius;
        
        if (forceScale > 1.0) forceScale = 1.0;
        
        velocity[i] += QPointF(
            force.x() * forceScale * 0.8,
            force.y() * forceScale * 0.8
        );
    }
    
    blobCenter += QPointF(force.x() * 0.2, force.y() * 0.2);
}