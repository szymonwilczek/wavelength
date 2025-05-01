#ifndef BLOBSTATE_H
#define BLOBSTATE_H

#include <QPointF>
#include <QVector>
#include <QVector2D>
#include "../blob_config.h"

class BlobState {
public:
    virtual ~BlobState() {}
    
    virtual void Apply(std::vector<QPointF>& control_points,
                      std::vector<QPointF>& velocity,
                      QPointF& blob_center,
                      const BlobConfig::BlobParameters& params) = 0;
                      
    virtual void ApplyForce(const QVector2D& force,
                           std::vector<QPointF>& velocity,
                           QPointF& blob_center,
                           const std::vector<QPointF>& control_points,
                           double blob_radius) = 0;
};

#endif // BLOBSTATE_H