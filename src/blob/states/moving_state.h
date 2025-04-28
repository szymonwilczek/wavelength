#ifndef MOVINGSTATE_H
#define MOVINGSTATE_H

#include "blob_state.h"
#include "../blob_config.h"
#include <QVector2D>

class MovingState final : public BlobState {
public:
    MovingState();
    
    void apply(std::vector<QPointF>& controlPoints, 
              std::vector<QPointF>& velocity,
              QPointF& blobCenter,
              const BlobConfig::BlobParameters& params) override;
              
    void applyForce(const QVector2D& force,
                   std::vector<QPointF>& velocity,
                   QPointF& blobCenter,
                   const std::vector<QPointF>& controlPoints,
                   double blobRadius) override;

    static void applyInertiaForce(std::vector<QPointF>& velocity,
                                  QPointF& blobCenter,
                                  const std::vector<QPointF>& controlPoints,
                                  double blobRadius,
                                  const QVector2D& windowVelocity);
};

#endif // MOVINGSTATE_H