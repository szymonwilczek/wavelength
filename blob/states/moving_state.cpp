#include "moving_state.h"
#include <QDebug>

MovingState::MovingState() {}

void MovingState::apply(std::vector<QPointF>& controlPoints, 
                       std::vector<QPointF>& velocity,
                       QPointF& blobCenter,
                       const BlobConfig::BlobParameters& params) {
    double avgVelX = 0.0;
    double avgVelY = 0.0;

    for (const auto& vel : velocity) {
        avgVelX += vel.x();
        avgVelY += vel.y();
    }
    avgVelX /= velocity.size();
    avgVelY /= velocity.size();

    if (std::abs(avgVelX) > 0.1 || std::abs(avgVelY) > 0.1) {
        QVector2D avgDirection(avgVelX, avgVelY);
        avgDirection.normalize();

        for (size_t i = 0; i < controlPoints.size(); ++i) {
            QPointF vectorFromCenter = controlPoints[i] - blobCenter;
            double distanceFromCenter = QVector2D(vectorFromCenter).length();

            double factor = distanceFromCenter / params.blobRadius;
            // Zmniejsz współczynnik odkształcenia (z 0.1 na 0.05)
            factor = factor * factor * 0.05;

            QPointF stretchForce = QPointF(avgVelX * factor, avgVelY * factor);
            // Zmniejsz wpływ siły (z 0.025 na 0.015)
            velocity[i] += stretchForce * 0.015;
        }
    }
}

void MovingState::applyInertiaForce(std::vector<QPointF>& velocity,
                                  QPointF& blobCenter,
                                  const std::vector<QPointF>& controlPoints,
                                  double blobRadius,
                                  const QVector2D& windowVelocity) {
    // Unikamy kosztownych obliczeń dla bardzo małych prędkości
    double windowSpeed = windowVelocity.length();
    if (windowSpeed < 0.1) {
        return;
    }

    // Zmniejsz współczynnik siły bezwładności z 0.005 na 0.0025
    QVector2D inertiaForce = -windowVelocity * (0.0025 * qMin(1.0, windowSpeed / 10.0));

    // Prekalkujemy dla wydajności
    double forceX = inertiaForce.x();
    double forceY = inertiaForce.y();

    // Prekalkujemy środek
    double centerX = blobCenter.x();
    double centerY = blobCenter.y();

    // Prekalkujemy odwrotność promienia
    double invRadius = 1.0 / blobRadius;

    // Zmniejsz wpływ sił na punkty kontrolne (z 0.4 na 0.2)
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        double distX = controlPoints[i].x() - centerX;
        double distY = controlPoints[i].y() - centerY;

        double distApprox = qAbs(distX) + qAbs(distY);
        double forceScale = qMin(distApprox * invRadius * 0.2, 0.3); // Zmniejszono oba współczynniki

        velocity[i].rx() += forceX * forceScale;
        velocity[i].ry() += forceY * forceScale;
    }

    // Zmniejsz wpływ na środek bloba (z 0.0005 na 0.0003)
    double centerFactor = 0.0003 * qMin(1.0, windowSpeed / 5.0);
    blobCenter.rx() += -windowVelocity.x() * centerFactor;
    blobCenter.ry() += -windowVelocity.y() * centerFactor;

    // Zmniejsz współczynnik dodatkowych sił dla gwałtownych ruchów
    if (windowSpeed > 5.0) {
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            double distX = controlPoints[i].x() - centerX;
            double distY = controlPoints[i].y() - centerY;

            double perpX = -forceY;
            double perpY = forceX;

            double dotProduct = distX * perpX + distY * perpY;
            // Zmniejszono z 0.00005 na 0.00002
            velocity[i].rx() += dotProduct * 0.00002;
            velocity[i].ry() += dotProduct * 0.00002;
        }
    }
}

void MovingState::applyForce(const QVector2D& force,
                           std::vector<QPointF>& velocity,
                           QPointF& blobCenter,
                           const std::vector<QPointF>& controlPoints,
                           double blobRadius) {
    
    const size_t numPoints = controlPoints.size();
    static std::vector<double> distanceCache;

    if (distanceCache.size() != numPoints) {
        distanceCache.resize(numPoints);
    }

    for (size_t i = 0; i < numPoints; ++i) {
        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        distanceCache[i] = QVector2D(vectorFromCenter).length();
    }

    QPointF forceXY(force.x(), force.y());
    QPointF centerForce = forceXY * 0.2;
    blobCenter += centerForce;

    for (size_t i = 0; i < numPoints; ++i) {
        double forceScale = qMin(distanceCache[i] / blobRadius, 1.0);
        velocity[i] += forceXY * (forceScale * 0.8);
    }
}