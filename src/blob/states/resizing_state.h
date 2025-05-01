#ifndef RESIZINGSTATE_H
#define RESIZINGSTATE_H

#include "blob_state.h"
#include "../blob_config.h"
#include <QVector2D>

class ResizingState final : public BlobState {
public:
    ResizingState();
    
    void Apply(std::vector<QPointF>& control_points,
              std::vector<QPointF>& velocity,
              QPointF& blob_center,
              const BlobConfig::BlobParameters& params) override;
              
    void ApplyForce(const QVector2D& force,
                   std::vector<QPointF>& velocity,
                   QPointF& blob_center,
                   const std::vector<QPointF>& control_points,
                   double blob_radius) override;
                   
    void HandleResize(std::vector<QPointF>& control_points,
                     std::vector<QPointF>& target_points,
                     std::vector<QPointF>& velocity,
                     QPointF& blob_center,
                     const QSize& old_size,
                     const QSize& new_size);
};

#endif // RESIZINGSTATE_H