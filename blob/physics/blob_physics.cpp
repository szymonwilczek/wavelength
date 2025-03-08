#include "blob_physics.h"
#include <QDebug>

BlobPhysics::BlobPhysics() {
    m_physicsTimer.start();
    m_threadPool.setMaxThreadCount(QThread::idealThreadCount());
}

void BlobPhysics::updatePhysicsParallel(std::vector<QPointF>& controlPoints,
                                      std::vector<QPointF>& targetPoints,
                                      std::vector<QPointF>& velocity,
                                      QPointF& blobCenter,
                                      const BlobConfig::BlobParameters& params,
                                      const BlobConfig::PhysicsParameters& physicsParams) {

    const size_t numPoints = controlPoints.size();
    const int numThreads = qMin(m_threadPool.maxThreadCount(), static_cast<int>(numPoints / 8 + 1));

    if (numPoints < 16 || numThreads <= 1) {
        updatePhysics(controlPoints, targetPoints, velocity, blobCenter, params, physicsParams, nullptr);
        return;
    }

    const double radiusThreshold = params.blobRadius * 1.1;
    const double dampingFactor = physicsParams.damping;
    const double velocityBlend = 0.8;
    const double prevVelocityBlend = 0.2;
    const double maxSpeed = params.blobRadius * physicsParams.maxSpeed;

    static std::vector<QPointF> previousVelocity(numPoints);
    std::copy(velocity.begin(), velocity.end(), previousVelocity.begin());

    std::atomic isInMotion(false);

    const size_t pointsPerThread = numPoints / numThreads;

    QVector<QFuture<void>> futures;
    futures.reserve(numThreads);

    for (int t = 0; t < numThreads; ++t) {
        const size_t startIdx = t * pointsPerThread;
        const size_t endIdx = (t == numThreads-1) ? numPoints : (t+1) * pointsPerThread;

        futures.append(QtConcurrent::run(&m_threadPool, [&, startIdx, endIdx]() {
            for (size_t i = startIdx; i < endIdx; ++i) {
                QPointF force = (targetPoints[i] - controlPoints[i]) * physicsParams.viscosity;

                QPointF vectorToCenter = blobCenter - controlPoints[i];
                double distFromCenter2 = vectorToCenter.x()*vectorToCenter.x() +
                                       vectorToCenter.y()*vectorToCenter.y();

                if (distFromCenter2 > radiusThreshold * radiusThreshold) {
                    double factor = 0.03 / qSqrt(distFromCenter2);
                    force.rx() += vectorToCenter.x() * factor;
                    force.ry() += vectorToCenter.y() * factor;
                }

                velocity[i] += force;

                velocity[i].rx() = velocity[i].x() * velocityBlend + previousVelocity[i].x() * prevVelocityBlend;
                velocity[i].ry() = velocity[i].y() * velocityBlend + previousVelocity[i].y() * prevVelocityBlend;

                velocity[i] *= dampingFactor;

                double speedSquared = velocity[i].x()*velocity[i].x() + velocity[i].y()*velocity[i].y();

                if (speedSquared < physicsParams.velocityThreshold * physicsParams.velocityThreshold) {
                    velocity[i] = QPointF(0, 0);
                } else {
                    isInMotion = true;

                    if (speedSquared > maxSpeed * maxSpeed) {
                        double scaleFactor = maxSpeed / qSqrt(speedSquared);
                        velocity[i] *= scaleFactor;
                    }
                }

                controlPoints[i] += velocity[i];
            }
        }));
    }

    for (auto& future : futures) {
        future.waitForFinished();
    }

    if (!isInMotion) {
        stabilizeBlob(controlPoints, blobCenter, params.blobRadius, physicsParams.stabilizationRate);
    }

    validateAndRepairControlPoints(controlPoints, velocity, blobCenter, params.blobRadius);
}

void BlobPhysics::updatePhysics(std::vector<QPointF>& controlPoints,
                              std::vector<QPointF>& targetPoints,
                              std::vector<QPointF>& velocity,
                              QPointF& blobCenter,
                              const BlobConfig::BlobParameters& params,
                              const BlobConfig::PhysicsParameters& physicsParams,
                              QWidget* parent) {

    static std::vector<QPointF> previousVelocity;
    if (previousVelocity.size() != velocity.size()) {
        previousVelocity.resize(velocity.size());
    }

    const double radiusThreshold = params.blobRadius * 1.1;
    const double dampingFactor = physicsParams.damping;
    const double velocityBlend = 0.8;
    const double prevVelocityBlend = 0.2;
    const double maxSpeed = params.blobRadius * physicsParams.maxSpeed;

    bool isInMotion = false;

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        previousVelocity[i] = velocity[i];

        QPointF force = (targetPoints[i] - controlPoints[i]) * physicsParams.viscosity;

        QPointF vectorToCenter = blobCenter - controlPoints[i];
        double distFromCenter2 = vectorToCenter.x()*vectorToCenter.x() + vectorToCenter.y()*vectorToCenter.y(); // square of distance

        if (distFromCenter2 > radiusThreshold * radiusThreshold) {
            double factor = 0.03 / qSqrt(distFromCenter2);
            force.rx() += vectorToCenter.x() * factor;
            force.ry() += vectorToCenter.y() * factor;
        }

        velocity[i] += force;

        // Miksowanie prędkości
        velocity[i].rx() = velocity[i].x() * velocityBlend + previousVelocity[i].x() * prevVelocityBlend;
        velocity[i].ry() = velocity[i].y() * velocityBlend + previousVelocity[i].y() * prevVelocityBlend;

        // Tłumienie
        velocity[i] *= dampingFactor;

        double speedSquared = velocity[i].x()*velocity[i].x() + velocity[i].y()*velocity[i].y();
        double thresholdSquared = physicsParams.velocityThreshold * physicsParams.velocityThreshold;

        if (speedSquared < thresholdSquared) {
            velocity[i] = QPointF(0, 0);
        } else {
            isInMotion = true;

            if (speedSquared > maxSpeed * maxSpeed) {
                double scaleFactor = maxSpeed / qSqrt(speedSquared);
                velocity[i] *= scaleFactor;
            }
        }

        controlPoints[i] += velocity[i];
    }

    if (!isInMotion) {
        stabilizeBlob(controlPoints, blobCenter, params.blobRadius, physicsParams.stabilizationRate);
    }

    if (Q_UNLIKELY(validateAndRepairControlPoints(controlPoints, velocity, blobCenter, params.blobRadius))) {
        qDebug() << "Wykryto nieprawidłowe punkty kontrolne. Zresetowano kształt blobu.";
    }
}

void BlobPhysics::initializeBlob(std::vector<QPointF>& controlPoints,
                               std::vector<QPointF>& targetPoints,
                               std::vector<QPointF>& velocity,
                               QPointF& blobCenter,
                               const BlobConfig::BlobParameters& params,
                               int width, int height) {

    blobCenter = QPointF(width / 2.0, height / 2.0);

    controlPoints = BlobMath::generateCircularPoints(blobCenter, params.blobRadius, params.numPoints);
    targetPoints = controlPoints;

    velocity.resize(params.numPoints);
    for (auto& vel : velocity) {
        vel = QPointF(0, 0);
    }
}

void BlobPhysics::handleBorderCollisions(std::vector<QPointF>& controlPoints,
                                       std::vector<QPointF>& velocity,
                                       QPointF& blobCenter,
                                       int width, int height,
                                       double restitution,
                                       int padding) {

    int minX = padding;
    int minY = padding;
    int maxX = width - padding;
    int maxY = height - padding;

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        // Kolizje w osi X
        if (controlPoints[i].x() < minX) {
            controlPoints[i].setX(minX);
            velocity[i].setX(-velocity[i].x() * restitution);
        }
        else if (controlPoints[i].x() > maxX) {
            controlPoints[i].setX(maxX);
            velocity[i].setX(-velocity[i].x() * restitution);
        }

        // Kolizje w osi Y
        if (controlPoints[i].y() < minY) {
            controlPoints[i].setY(minY);
            velocity[i].setY(-velocity[i].y() * restitution);
        }
        else if (controlPoints[i].y() > maxY) {
            controlPoints[i].setY(maxY);
            velocity[i].setY(-velocity[i].y() * restitution);
        }
    }

    if (blobCenter.x() < minX) blobCenter.setX(minX);
    if (blobCenter.x() > maxX) blobCenter.setX(maxX);
    if (blobCenter.y() < minY) blobCenter.setY(minY);
    if (blobCenter.y() > maxY) blobCenter.setY(maxY);
}

void BlobPhysics::constrainNeighborDistances(std::vector<QPointF>& controlPoints,
                                           std::vector<QPointF>& velocity,
                                           double minDistance,
                                           double maxDistance) {

    if (controlPoints.empty()) return;

    int numPoints = controlPoints.size();

    for (int i = 0; i < numPoints; ++i) {
        int next = (i + 1) % numPoints;

        QPointF diff = controlPoints[next] - controlPoints[i];
        double distance = QVector2D(diff).length();

        if (distance < minDistance || distance > maxDistance) {
            QPointF direction = diff / distance;
            double targetDistance = BlobMath::clamp(distance, minDistance, maxDistance);

            QPointF correction = direction * (distance - targetDistance) * 0.5;
            controlPoints[i] += correction;
            controlPoints[next] -= correction;

            if (distance < minDistance) {
                velocity[i] -= direction * 0.3;
                velocity[next] += direction * 0.3;
            } else if (distance > maxDistance) {
                velocity[i] += direction * 0.3;
                velocity[next] -= direction * 0.3;
            }
        }
    }
}

void BlobPhysics::smoothBlobShape(std::vector<QPointF>& controlPoints) {
    if (controlPoints.empty()) return;

    int numPoints = controlPoints.size();
    std::vector<QPointF> smoothedPoints = controlPoints;

    for (int i = 0; i < numPoints; ++i) {
        int prev = (i + numPoints - 1) % numPoints;
        int next = (i + 1) % numPoints;

        QPointF neighborAverage = (controlPoints[prev] + controlPoints[next]) * 0.5;

        smoothedPoints[i] += (neighborAverage - controlPoints[i]) * 0.15;
    }

    controlPoints = smoothedPoints;
}

void BlobPhysics::stabilizeBlob(std::vector<QPointF>& controlPoints,
                              const QPointF& blobCenter,
                              double blobRadius,
                              double stabilizationRate) {
    int numPoints = controlPoints.size();

    double avgDistance = 0.0;
    for (int i = 0; i < numPoints; ++i) {
        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        avgDistance += QVector2D(vectorFromCenter).length();
    }
    avgDistance /= numPoints;

    double radiusRatio = blobRadius / avgDistance;

    static std::vector<double> randomOffsets;
    if (randomOffsets.size() != numPoints) {
        randomOffsets.resize(numPoints);
        for (int i = 0; i < numPoints; ++i) {
            randomOffsets[i] = 0.95 + 0.1 * (qrand() % 100) / 100.0;
        }
    }

    for (int i = 0; i < numPoints; ++i) {
        double angle = 2 * M_PI * i / numPoints;

        QPointF idealPoint(
            blobCenter.x() + blobRadius * randomOffsets[i] * qCos(angle),
            blobCenter.y() + blobRadius * randomOffsets[i] * qSin(angle)
        );

        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        double currentDistance = QVector2D(vectorFromCenter).length();

        if (radiusRatio < 0.9 || radiusRatio > 1.1) {
            double correctedDistance = currentDistance * radiusRatio;
            if (currentDistance > 0.001) {
                QPointF normalizedVector = vectorFromCenter * (1.0 / currentDistance);
                controlPoints[i] = blobCenter + normalizedVector * correctedDistance;
            }
        }

        controlPoints[i] += (idealPoint - controlPoints[i]) * stabilizationRate;
    }
}

bool BlobPhysics::validateAndRepairControlPoints(std::vector<QPointF>& controlPoints,
                                               std::vector<QPointF>& velocity,
                                               const QPointF& blobCenter,
                                               double blobRadius) {

    bool hasInvalidPoints = false;

    for (size_t i = 0; i < controlPoints.size(); ++i) {
        if (!BlobMath::isValidPoint(controlPoints[i]) || !BlobMath::isValidPoint(velocity[i])) {
            hasInvalidPoints = true;
            break;
        }
    }

    if (hasInvalidPoints) {
        int numPoints = controlPoints.size();

        for (int i = 0; i < numPoints; ++i) {
            double angle = 2 * M_PI * i / numPoints;
            double randomRadius = blobRadius * (0.9 + 0.2 * (qrand() % 100) / 100.0);

            controlPoints[i] = QPointF(
                blobCenter.x() + randomRadius * qCos(angle),
                blobCenter.y() + randomRadius * qSin(angle)
            );

            velocity[i] = QPointF(0, 0);
        }
        return true;
    }

    return false;
}

QVector2D BlobPhysics::calculateWindowVelocity(const QPointF& currentPos) {
    double deltaTime = m_physicsTimer.restart() / 1000.0;

    if (deltaTime < 0.001) {
        deltaTime = 0.016; // ~~60 FPS
    } else if (deltaTime > 0.1) {
        deltaTime = 0.1;  // min 10 FPS
    }

    QVector2D windowVelocity(
        (currentPos.x() - m_lastWindowPos.x()) / deltaTime,
        (currentPos.y() - m_lastWindowPos.y()) / deltaTime
    );

    static QVector2D lastVelocity(0, 0);

    windowVelocity = windowVelocity * 0.7 + lastVelocity * 0.3;
    lastVelocity = windowVelocity;

    const double maxVelocity = 3000.0;
    if (windowVelocity.length() > maxVelocity) {
        windowVelocity = windowVelocity.normalized() * maxVelocity;
    }

    m_lastWindowVelocity = windowVelocity;

    return windowVelocity;
}

void BlobPhysics::setLastWindowPos(const QPointF& pos) {
    m_lastWindowPos = pos;
}

QPointF BlobPhysics::getLastWindowPos() const {
    return m_lastWindowPos;
}

QVector2D BlobPhysics::getLastWindowVelocity() const {
    return m_lastWindowVelocity;
}