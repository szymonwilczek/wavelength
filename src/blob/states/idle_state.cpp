#include "idle_state.h"
#include <cmath>
#include <qmath.h>
#include <QDebug>

IdleState::IdleState() {
    m_mainPhaseOffset = 0.0;
}

void IdleState::updatePhases() {
    // Przyspieszenie fal - zwiększamy inkrementację
    m_idleParams.wavePhase += 0.01;  // Było 0.005
    if (m_idleParams.wavePhase > 2 * M_PI) {
        m_idleParams.wavePhase -= 2 * M_PI;
    }

    m_secondPhase += 0.015;  // Było 0.008
    if (m_secondPhase > 2 * M_PI) {
        m_secondPhase -= 2 * M_PI;
    }

    m_rotationPhase += 0.003;  // Było 0.002
    if (m_rotationPhase > 2 * M_PI) {
        m_rotationPhase -= 2 * M_PI;
    }
}

void IdleState::resetInitialization() {
    m_isInitializing = true;
    m_heartbeatCount = 0;
    m_heartbeatPhase = 0.0;
}

void IdleState::apply(std::vector<QPointF>& controlPoints,
                      std::vector<QPointF>& velocity,
                      QPointF& blobCenter,
                      const BlobConfig::BlobParameters& params) {

    // Sprawdzamy, czy jesteśmy w fazie inicjalizacji
    if (m_isInitializing) {
        applyHeartbeatEffect(controlPoints, velocity, blobCenter, params);
        return;
    }

    // Standardowa animacja idle z falami - istniejący kod
    updatePhases();

    const double rotationStrength = 0.15 * std::sin(m_rotationPhase * 0.3);

    const QPointF originalCenter = blobCenter;
    const QPointF screenCenter(params.screenWidth / 2.0, params.screenHeight / 2.0);

    // POPRAWKA: Delikatna siła przyciągająca do środka ekranu
    const QPointF centeringForce = (screenCenter - blobCenter) * 0.01;
    blobCenter += centeringForce;

    QPointF totalDisplacement(0, 0);

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        const double angle = std::atan2(vectorFromCenter.y(), vectorFromCenter.x());
        const double distanceFromCenter = QVector2D(vectorFromCenter).length();

        // Fala 1: główna fala obwodowa
        double waveStrength = m_idleParams.waveAmplitude * 0.9 *
            std::sin(m_idleParams.wavePhase + m_idleParams.waveFrequency * angle);

        // Fala 2: dodatkowa fala z inną częstotliwością dla dodania głębi
        waveStrength += m_idleParams.waveAmplitude * 0.5 *
            std::sin(m_secondPhase + m_idleParams.waveFrequency * 2.0 * angle);

        // // Fala 3: fala z zależnością od czasu ale nie od pozycji
        // waveStrength += m_idleParams.waveAmplitude * 0.3 *
        //     std::sin(m_idleParams.wavePhase * 0.7);

        QVector2D normalizedVector = QVector2D(vectorFromCenter).normalized();
        QPointF perpVector(-normalizedVector.y(), normalizedVector.x());

        const double rotationFactor = rotationStrength * (distanceFromCenter / params.blobRadius);
        QPointF rotationForce = rotationFactor * perpVector;

        QPointF forceVector = QPointF(normalizedVector.x(), normalizedVector.y()) * waveStrength + rotationForce;

        double forceScale = 0.2 + 0.8 * (distanceFromCenter / params.blobRadius);
        if (forceScale > 1.0) forceScale = 1.0;

        const QPointF forceDelta = forceVector * forceScale * 0.15;
        velocity[i] += forceDelta;

        totalDisplacement += forceDelta;
    }

    const QPointF avgDisplacement = totalDisplacement / controlPoints.size();
    for (auto& vel : velocity) {
        vel -= avgDisplacement;
    }
    if (QVector2D(blobCenter - screenCenter).length() > params.blobRadius * 0.1) {
        blobCenter = blobCenter * 0.95 + screenCenter * 0.05;
    } else {
        blobCenter = originalCenter;
    }
}

void IdleState::applyForce(const QVector2D& force,
                         std::vector<QPointF>& velocity,
                         QPointF& blobCenter,
                         const std::vector<QPointF>& controlPoints,
                         const double blobRadius) {

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        const double distanceFromCenter = QVector2D(vectorFromCenter).length();

        double forceScale = distanceFromCenter / blobRadius;
        if (forceScale > 1.0) forceScale = 1.0;

        velocity[i] += QPointF(
            force.x() * forceScale * 0.8,
            force.y() * forceScale * 0.8
        );
    }

    blobCenter += QPointF(force.x() * 0.2, force.y() * 0.2);
}

void IdleState::applyHeartbeatEffect(const std::vector<QPointF>& controlPoints,
                                    std::vector<QPointF>& velocity,
                                    const QPointF& blobCenter,
                                    const BlobConfig::BlobParameters& params) {
    // Zmniejsz wartość, aby zwolnić tempo bicia serca
    m_heartbeatPhase += 0.02;  // Zwolnione z 0.03 na 0.015

    if (m_heartbeatPhase >= 2 * M_PI) {
        m_heartbeatPhase -= 2 * M_PI;
        m_heartbeatCount++;

        if (m_heartbeatCount >= REQUIRED_HEARTBEATS) {
            m_isInitializing = false;
            return;
        }
    }

    // Funkcja pulsu serca z mniejszymi wartościami dla bardziej subtelnego efektu
    double pulseStrength;

    if (m_heartbeatPhase < M_PI * 0.25) {
        // Szybki skurcz - zmniejszony z 1.0 na 0.5
        pulseStrength = 0.5 * std::sin(m_heartbeatPhase * 4);
    } else if (m_heartbeatPhase < M_PI * 0.5) {
        // Krótka pauza - zmniejszona z 0.3 na 0.2
        pulseStrength = 0.3 * std::sin(m_heartbeatPhase * 4);
    } else if (m_heartbeatPhase < M_PI * 0.75) {
        // Powolny rozkurcz - zmniejszony z 0.8 na 0.4
        pulseStrength = 0.4 * std::sin((m_heartbeatPhase - M_PI * 0.5) * 0.8);
    } else {
        // Długa pauza przed następnym biciem - zmniejszona i przyspieszona
        // Zmiana z 0.5 na 0.2 i dodanie mnożnika 1.2 dla kąta, aby skrócić pauzę
        pulseStrength = 0.5 * std::sin(m_heartbeatPhase * 1.2);
    }

    // Stosujemy efekt pulsu do wszystkich punktów kontrolnych
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        QVector2D normalizedVector = QVector2D(vectorFromCenter).normalized();

        // Siła pulsu - ekspansja i kontrakcja
        const double distanceRatio = QVector2D(vectorFromCenter).length() / params.blobRadius;
        // Zmniejszamy mnożnik z 0.8 na 0.5 dla subtelniejszego efektu
        const double scaledPulse = pulseStrength * distanceRatio * 0.5;

        const QPointF pulseForce = QPointF(normalizedVector.x(), normalizedVector.y()) * scaledPulse;
        velocity[i] += pulseForce;
    }
}