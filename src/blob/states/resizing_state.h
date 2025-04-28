#ifndef RESIZINGSTATE_H
#define RESIZINGSTATE_H

#include "blob_state.h"
#include "../blob_config.h"
#include <QVector2D>

class ResizingState final : public BlobState {
public:
    ResizingState();
    
    void apply(std::vector<QPointF>& controlPoints, 
              std::vector<QPointF>& velocity,
              QPointF& blobCenter,
              const BlobConfig::BlobParameters& params) override;
              
    void applyForce(const QVector2D& force,
                   std::vector<QPointF>& velocity,
                   QPointF& blobCenter,
                   const std::vector<QPointF>& controlPoints,
                   double blobRadius) override;
                   
    void handleResize(std::vector<QPointF>& controlPoints,
                     std::vector<QPointF>& targetPoints,
                     std::vector<QPointF>& velocity,
                     QPointF& blobCenter,
                     const QSize& oldSize, 
                     const QSize& newSize);
};

#endif // RESIZINGSTATE_H