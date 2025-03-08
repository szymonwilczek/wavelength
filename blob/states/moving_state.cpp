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
        QVector2D perpendicular(-avgVelY, avgVelX);

        if (avgDirection.length() > 0) {
            avgDirection.normalize();
        }
        if (perpendicular.length() > 0) {
            perpendicular.normalize();
        }

        for (size_t i = 0; i < controlPoints.size(); ++i) {
            QPointF vectorFromCenter = controlPoints[i] - blobCenter;
            double distanceFromCenter = QVector2D(vectorFromCenter).length();

            double factor = distanceFromCenter / params.blobRadius;
            factor = factor * factor * 0.2;

            QPointF stretchForce = QPointF(avgVelX * factor, avgVelY * factor);
            velocity[i] += stretchForce * 0.05;

            QVector2D normalizedFromCenter = QVector2D(vectorFromCenter);
            if (normalizedFromCenter.length() > 0) {
                normalizedFromCenter.normalize();
            }
            double dotProduct = normalizedFromCenter.x() * avgDirection.x() +
                               normalizedFromCenter.y() * avgDirection.y();

            QPointF squeezeForce = QPointF(perpendicular.x() * factor * dotProduct,
                                          perpendicular.y() * factor * dotProduct);
            velocity[i] += squeezeForce * 0.02;
        }
    }
}

void MovingState::applyInertiaForce(std::vector<QPointF>& velocity,
                                  QPointF& blobCenter,
                                  const std::vector<QPointF>& controlPoints,
                                  double blobRadius,
                                  const QVector2D& windowVelocity) {

    QVector2D inertiaForce = -windowVelocity * 0.01;


    applyForce(inertiaForce, velocity, blobCenter, controlPoints, blobRadius);

    double centerFactor = 0.001;
    QPointF centerForce(-windowVelocity.x() * centerFactor, -windowVelocity.y() * centerFactor);
    blobCenter += centerForce;

    double avgVelX = 0.0, avgVelY = 0.0;
    for (const auto& vel : velocity) {
        avgVelX += vel.x();
        avgVelY += vel.y();
    }
    if (!velocity.empty()) {
        avgVelX /= velocity.size();
        avgVelY /= velocity.size();
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