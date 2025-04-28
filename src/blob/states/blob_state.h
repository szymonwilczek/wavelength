#ifndef BLOBSTATE_H
#define BLOBSTATE_H

#include <QPointF>
#include <QVector>
#include <QVector2D>
#include "../blob_config.h"

class BlobState {
public:
    virtual ~BlobState() {}
    
    virtual void apply(std::vector<QPointF>& controlPoints,
                      std::vector<QPointF>& velocity,
                      QPointF& blobCenter,
                      const BlobConfig::BlobParameters& params) = 0;
                      
    virtual void applyForce(const QVector2D& force,
                           std::vector<QPointF>& velocity,
                           QPointF& blobCenter,
                           const std::vector<QPointF>& controlPoints,
                           double blobRadius) = 0;
};

#endif // BLOBSTATE_H