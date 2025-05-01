#ifndef MOVINGSTATE_H
#define MOVINGSTATE_H

#include "blob_state.h"
#include "../blob_config.h"
#include <QVector2D>

class MovingState final : public BlobState {
public:
    MovingState();
    
    void Apply(std::vector<QPointF>& control_points,
              std::vector<QPointF>& velocity,
              QPointF& blob_center,
              const BlobConfig::BlobParameters& params) override;
              
    void ApplyForce(const QVector2D& force,
                   std::vector<QPointF>& velocity,
                   QPointF& blob_center,
                   const std::vector<QPointF>& control_points,
                   double blob_radius) override;

    static void ApplyInertiaForce(std::vector<QPointF>& velocity,
                                  QPointF& blob_center,
                                  const std::vector<QPointF>& control_points,
                                  double blob_radius,
                                  const QVector2D& window_velocity);
};

#endif // MOVINGSTATE_H