#include "blob_physics.h"
#include <QDebug>

BlobPhysics::BlobPhysics() {
    m_physicsTimer.start();
    m_threadPool.setMaxThreadCount(QThread::idealThreadCount());
}

void BlobPhysics::updatePhysicsOptimized(std::vector<QPointF>& controlPoints,
                                       std::vector<QPointF>& targetPoints,
                                       std::vector<QPointF>& velocity,
                                       QPointF& blobCenter,
                                       const BlobConfig::BlobParameters& params,
                                       const BlobConfig::PhysicsParameters& physicsParams) {
    const size_t numPoints = controlPoints.size();

    // Używamy struktur danych zoptymalizowanych pod kątem wydajności pamięci podręcznej
    static std::vector<float> posX, posY, targX, targY, velX, velY, forceX, forceY;

    // Rezerwuj pamięć raz
    if (posX.size() != numPoints) {
        posX.resize(numPoints);
        posY.resize(numPoints);
        targX.resize(numPoints);
        targY.resize(numPoints);
        velX.resize(numPoints);
        velY.resize(numPoints);
        forceX.resize(numPoints);
        forceY.resize(numPoints);
    }

    // Konwertuj dane na zoptymalizowane struktury (SoA - Structure of Arrays)
    const float centerX = static_cast<float>(blobCenter.x());
    const float centerY = static_cast<float>(blobCenter.y());
    const float radiusThreshold = static_cast<float>(params.blobRadius * 1.1f);
    const float radiusThresholdSquared = radiusThreshold * radiusThreshold;
    const float dampingFactor = static_cast<float>(physicsParams.damping);
    const float viscosity = static_cast<float>(physicsParams.viscosity);
    const float velocityThreshold = static_cast<float>(physicsParams.velocityThreshold);
    const float velocityThresholdSquared = velocityThreshold * velocityThreshold;
    const float maxSpeed = static_cast<float>(params.blobRadius * physicsParams.maxSpeed);
    const float maxSpeedSquared = maxSpeed * maxSpeed;

    // Kopiuj dane do zoptymalizowanych buforów
    #pragma omp parallel for
    for (int i = 0; i < numPoints; ++i) {
        posX[i] = static_cast<float>(controlPoints[i].x());
        posY[i] = static_cast<float>(controlPoints[i].y());
        targX[i] = static_cast<float>(targetPoints[i].x());
        targY[i] = static_cast<float>(targetPoints[i].y());
        velX[i] = static_cast<float>(velocity[i].x());
        velY[i] = static_cast<float>(velocity[i].y());
        forceX[i] = 0.0f;
        forceY[i] = 0.0f;
    }

    bool isInMotion = false;

    // Przetwarzaj w blokach dla lepszej wydajności cache
    const int blockSize = 64;  // Zoptymalizowane pod kątem linii cache

    #pragma omp parallel for reduction(|:isInMotion)
    for (int blockStart = 0; blockStart < numPoints; blockStart += blockSize) {
        const int blockEnd = std::min(blockStart + blockSize, static_cast<int>(numPoints));

        for (int i = blockStart; i < blockEnd; ++i) {
            // Oblicz siłę
            forceX[i] = (targX[i] - posX[i]) * viscosity;
            forceY[i] = (targY[i] - posY[i]) * viscosity;

            // Oblicz wektor do środka
            const float vecToCenterX = centerX - posX[i];
            const float vecToCenterY = centerY - posY[i];
            const float distSquared = vecToCenterX * vecToCenterX + vecToCenterY * vecToCenterY;

            if (distSquared > radiusThresholdSquared) {
                // Fast inverse square root approximation
                const float invDist = 1.0f / sqrtf(distSquared);
                const float factor = 0.03f * invDist;
                forceX[i] += vecToCenterX * factor;
                forceY[i] += vecToCenterY * factor;
            }

            // Aktualizuj prędkość
            velX[i] += forceX[i];
            velY[i] += forceY[i];

            // Zastosuj tłumienie
            velX[i] *= dampingFactor;
            velY[i] *= dampingFactor;

            // Sprawdź prędkość
            const float speedSquared = velX[i] * velX[i] + velY[i] * velY[i];

            if (speedSquared < velocityThresholdSquared) {
                velX[i] = 0.0f;
                velY[i] = 0.0f;
            } else {
                isInMotion = true;

                if (speedSquared > maxSpeedSquared) {
                    const float scaleFactor = maxSpeed / sqrtf(speedSquared);
                    velX[i] *= scaleFactor;
                    velY[i] *= scaleFactor;
                }
            }

            // Aktualizuj pozycję
            posX[i] += velX[i];
            posY[i] += velY[i];
        }
    }

    // Kopiuj dane z powrotem
    #pragma omp parallel for
    for (int i = 0; i < numPoints; ++i) {
        controlPoints[i].rx() = posX[i];
        controlPoints[i].ry() = posY[i];
        velocity[i].rx() = velX[i];
        velocity[i].ry() = velY[i];
    }

    if (!isInMotion) {
        stabilizeBlob(controlPoints, blobCenter, params.blobRadius, physicsParams.stabilizationRate);
    }

    validateAndRepairControlPoints(controlPoints, velocity, blobCenter, params.blobRadius);
}

void BlobPhysics::updatePhysicsParallel(std::vector<QPointF>& controlPoints,
                                      std::vector<QPointF>& targetPoints,
                                      std::vector<QPointF>& velocity,
                                      QPointF& blobCenter,
                                      const BlobConfig::BlobParameters& params,
                                      const BlobConfig::PhysicsParameters& physicsParams) {

    const size_t numPoints = controlPoints.size();

    // Użyj SIMD-friendly struktury danych gdy liczba punktów jest duża
    if (numPoints > 64) {
        updatePhysicsOptimized(controlPoints, targetPoints, velocity, blobCenter, params, physicsParams);
        return;
    }

    const int numThreads = qMin(m_threadPool.maxThreadCount(), static_cast<int>(numPoints / 8 + 1));
    const int batchSize = 16; // Zwiększony rozmiar partii dla lepszej wydajności cache

    if (numPoints < 24 || numThreads <= 1) {
        updatePhysics(controlPoints, targetPoints, velocity, blobCenter, params, physicsParams, nullptr);
        return;
    }

    // Prekalkujemy stałe wartości dla wszystkich wątków
    const double radiusThreshold = params.blobRadius * 1.1;
    const double radiusThresholdSquared = radiusThreshold * radiusThreshold;
    const double dampingFactor = physicsParams.damping;
    const double velocityBlend = 0.8;
    const double prevVelocityBlend = 0.2;
    const double maxSpeed = params.blobRadius * physicsParams.maxSpeed;
    const double velocityThresholdSquared = physicsParams.velocityThreshold * physicsParams.velocityThreshold;
    const double maxSpeedSquared = maxSpeed * maxSpeed;

    // Użyj statycznego bufora dla poprzednich prędkości, aby uniknąć realokacji
    static std::vector<QPointF> previousVelocity;
    if (previousVelocity.size() != numPoints) {
        previousVelocity.resize(numPoints);
    }
    std::copy(velocity.begin(), velocity.end(), previousVelocity.begin());

    std::atomic<bool> isInMotion(false);

    QVector<QFuture<void>> futures;
    futures.reserve(numThreads);

    for (int t = 0; t < numThreads; ++t) {
        const size_t startIdx = t * (numPoints / numThreads);
        const size_t endIdx = (t == numThreads-1) ? numPoints : (t+1) * (numPoints / numThreads);

        futures.append(QtConcurrent::run(&m_threadPool, [&, startIdx, endIdx, radiusThresholdSquared,
                                        dampingFactor, velocityBlend, prevVelocityBlend,
                                        velocityThresholdSquared, maxSpeedSquared]() {
            // Prekalkujemy środek jeden raz dla każdego wątku
            const double centerX = blobCenter.x();
            const double centerY = blobCenter.y();

            for (size_t batchStart = startIdx; batchStart < endIdx; batchStart += batchSize) {
                const size_t batchEnd = qMin(batchStart + batchSize, endIdx);

                for (size_t i = batchStart; i < batchEnd; ++i) {
                    QPointF& currentPoint = controlPoints[i];
                    QPointF& currentTarget = targetPoints[i];
                    QPointF& currentVelocity = velocity[i];
                    const QPointF& prevVelocity = previousVelocity[i];

                    // Oblicz siłę bezpośrednio bez tworzenia pośrednich obiektów
                    double forceX = (currentTarget.x() - currentPoint.x()) * physicsParams.viscosity;
                    double forceY = (currentTarget.y() - currentPoint.y()) * physicsParams.viscosity;

                    // Oblicz wektor do środka bez tworzenia obiektu QPointF
                    double vectorToCenterX = centerX - currentPoint.x();
                    double vectorToCenterY = centerY - currentPoint.y();
                    double distFromCenter2 = vectorToCenterX*vectorToCenterX + vectorToCenterY*vectorToCenterY;

                    if (distFromCenter2 > radiusThresholdSquared) {
                        // Fast inverse square root approximation można zastosować zamiast qSqrt dla wyższej wydajności
                        double factor = 0.03 / qSqrt(distFromCenter2);
                        forceX += vectorToCenterX * factor;
                        forceY += vectorToCenterY * factor;
                    }

                    currentVelocity.rx() += forceX;
                    currentVelocity.ry() += forceY;

                    // Blend prędkości
                    currentVelocity.rx() = currentVelocity.x() * velocityBlend + prevVelocity.x() * prevVelocityBlend;
                    currentVelocity.ry() = currentVelocity.y() * velocityBlend + prevVelocity.y() * prevVelocityBlend;

                    // Zastosuj tłumienie
                    currentVelocity *= dampingFactor;

                    double speedSquared = currentVelocity.x()*currentVelocity.x() +
                                         currentVelocity.y()*currentVelocity.y();

                    if (speedSquared < velocityThresholdSquared) {
                        currentVelocity = QPointF(0, 0);
                    } else {
                        isInMotion = true;

                        if (speedSquared > maxSpeedSquared) {
                            double scaleFactor = maxSpeed / qSqrt(speedSquared);
                            currentVelocity *= scaleFactor;
                        }
                    }

                    // Aktualizuj pozycję punktu
                    currentPoint.rx() += currentVelocity.x();
                    currentPoint.ry() += currentVelocity.y();
                }
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