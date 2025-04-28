#ifndef BLOBPHYSICS_H
#define BLOBPHYSICS_H

#include <QtConcurrent>
#include <QThreadPool>
#include <QTime>
#include "../blob_config.h"
#include "../utils/blob_math.h"

class BlobPhysics {
public:
    BlobPhysics();

    void updatePhysicsOptimized(std::vector<QPointF> &controlPoints, const std::vector<QPointF> &targetPoints,
                                std::vector<QPointF> &velocity, const QPointF &blobCenter,
                                const BlobConfig::BlobParameters &params,
                                const BlobConfig::PhysicsParameters &physicsParams);

    void updatePhysicsParallel(std::vector<QPointF> &controlPoints, std::vector<QPointF> &targetPoints,
                               std::vector<QPointF> &velocity, QPointF &blobCenter,
                               const BlobConfig::BlobParameters &params,
                               const BlobConfig::PhysicsParameters &physicsParams);

    void updatePhysics(std::vector<QPointF> &controlPoints,
                       const std::vector<QPointF> &targetPoints,
                       std::vector<QPointF> &velocity,
                       const QPointF &blobCenter,
                       const BlobConfig::BlobParameters &params,
                       const BlobConfig::PhysicsParameters &physicsParams);

    static void initializeBlob(std::vector<QPointF>& controlPoints,
                               std::vector<QPointF>& targetPoints,
                               std::vector<QPointF>& velocity,
                               QPointF& blobCenter,
                               const BlobConfig::BlobParameters& params,
                               int width, int height);

    static void handleBorderCollisions(std::vector<QPointF>& controlPoints,
                                       std::vector<QPointF>& velocity,
                                       QPointF& blobCenter,
                                       int width, int height,
                                       double restitution,
                                       int padding);

    static void constrainNeighborDistances(std::vector<QPointF>& controlPoints,
                                           std::vector<QPointF>& velocity,
                                           double minDistance,
                                           double maxDistance);

    static void smoothBlobShape(std::vector<QPointF>& controlPoints);

    static void stabilizeBlob(std::vector<QPointF>& controlPoints,
                              const QPointF& blobCenter,
                              double blobRadius,
                              double stabilizationRate);

    static bool validateAndRepairControlPoints(std::vector<QPointF>& controlPoints,
                                               std::vector<QPointF>& velocity,
                                               const QPointF& blobCenter,
                                               double blobRadius);

    QVector2D calculateWindowVelocity(const QPointF& currentPos);

    void setLastWindowPos(const QPointF& pos);


    QPointF getLastWindowPos() const;
    QVector2D getLastWindowVelocity() const;

private:
    QTime m_physicsTimer;
    QPointF m_lastWindowPos;
    QVector2D m_lastWindowVelocity;
    QThreadPool m_threadPool;
};

#endif // BLOBPHYSICS_H