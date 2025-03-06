#include "idle_state.h"
#include <cmath>
#include <qmath.h>
#include <QDebug>

IdleState::IdleState() {
    m_mainPhaseOffset = 0.0;
}

void IdleState::updatePhases() {
    m_idleParams.wavePhase += 0.005;
    if (m_idleParams.wavePhase > 2 * M_PI) {
        m_idleParams.wavePhase -= 2 * M_PI;
    }

    m_secondPhase += 0.008;
    if (m_secondPhase > 2 * M_PI) {
        m_secondPhase -= 2 * M_PI;
    }

    m_rotationPhase += 0.002;
    if (m_rotationPhase > 2 * M_PI) {
        m_rotationPhase -= 2 * M_PI;
    }
}

void IdleState::apply(std::vector<QPointF>& controlPoints,
                     std::vector<QPointF>& velocity,
                     QPointF& blobCenter,
                     const BlobConfig::BlobParameters& params) {

    updatePhases();

    double rotationStrength = 0.15 * std::sin(m_rotationPhase * 0.3);

    QPointF originalCenter = blobCenter;

    QPointF totalDisplacement(0, 0);

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        double angle = std::atan2(vectorFromCenter.y(), vectorFromCenter.x());
        double distanceFromCenter = QVector2D(vectorFromCenter).length();

        // Fala 1: główna fala obwodowa
        double waveStrength = m_idleParams.waveAmplitude * 0.9 *
            std::sin(m_idleParams.wavePhase + m_idleParams.waveFrequency * angle);

        // Fala 2: dodatkowa fala z inną częstotliwością dla dodania głębi
        waveStrength += m_idleParams.waveAmplitude * 0.5 *
            std::sin(m_secondPhase + m_idleParams.waveFrequency * 2.0 * angle);

        // Fala 3: fala z zależnością od czasu ale nie od pozycji
        waveStrength += m_idleParams.waveAmplitude * 0.3 *
            std::sin(m_idleParams.wavePhase * 0.7);

        QVector2D normalizedVector = QVector2D(vectorFromCenter).normalized();
        QPointF perpVector(-normalizedVector.y(), normalizedVector.x());

        double rotationFactor = rotationStrength * (distanceFromCenter / params.blobRadius);
        QPointF rotationForce = rotationFactor * perpVector;

        QPointF forceVector = QPointF(normalizedVector.x(), normalizedVector.y()) * waveStrength + rotationForce;

        double forceScale = 0.2 + 0.8 * (distanceFromCenter / params.blobRadius);
        if (forceScale > 1.0) forceScale = 1.0;

        QPointF forceDelta = forceVector * forceScale * 0.15;
        velocity[i] += forceDelta;

        totalDisplacement += forceDelta;
    }

    QPointF avgDisplacement = totalDisplacement / controlPoints.size();
    for (auto& vel : velocity) {
        vel -= avgDisplacement;
    }

    blobCenter = originalCenter;
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