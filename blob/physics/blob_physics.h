#ifndef BLOBPHYSICS_H
#define BLOBPHYSICS_H

#include <QPointF>
#include <QVector>
#include <QVector2D>
#include <QTime>
#include <QWidget>
#include "../core/blob_config.h"
#include "../utils/blob_math.h"

class BlobPhysics {
public:
    BlobPhysics();

    void updatePhysics(std::vector<QPointF>& controlPoints,
                       std::vector<QPointF>& targetPoints,
                       std::vector<QPointF>& velocity,
                       QPointF& blobCenter,
                       const BlobConfig::BlobParameters& params,
                       const BlobConfig::PhysicsParameters& physicsParams,
                       QWidget* parent);

    void initializeBlob(std::vector<QPointF>& controlPoints,
                        std::vector<QPointF>& targetPoints,
                        std::vector<QPointF>& velocity,
                        QPointF& blobCenter,
                        const BlobConfig::BlobParameters& params,
                        int width, int height);

    void handleBorderCollisions(std::vector<QPointF>& controlPoints,
                                std::vector<QPointF>& velocity,
                                QPointF& blobCenter,
                                int width, int height,
                                double restitution,
                                int padding);

    void constrainNeighborDistances(std::vector<QPointF>& controlPoints,
                                   std::vector<QPointF>& velocity,
                                   double minDistance,
                                   double maxDistance);

    void smoothBlobShape(std::vector<QPointF>& controlPoints);

    void stabilizeBlob(std::vector<QPointF>& controlPoints,
                      const QPointF& blobCenter,
                      double blobRadius,
                      double stabilizationRate);

    bool validateAndRepairControlPoints(std::vector<QPointF>& controlPoints,
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
};

#endif // BLOBPHYSICS_H