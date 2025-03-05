#include "resizing_state.h"
#include <QVector2D>

ResizingState::ResizingState() {}

void ResizingState::apply(std::vector<QPointF>& controlPoints, 
                         std::vector<QPointF>& velocity,
                         QPointF& blobCenter,
                         const BlobConfig::BlobParameters& params) {
}

void ResizingState::handleResize(std::vector<QPointF>& controlPoints,
                               std::vector<QPointF>& targetPoints,
                               std::vector<QPointF>& velocity,
                               QPointF& blobCenter,
                               const QSize& oldSize, 
                               const QSize& newSize) {
    
    QPointF oldCenter = blobCenter;
    
    blobCenter = QPointF(newSize.width() / 2.0, newSize.height() / 2.0);
    
    QPointF delta = blobCenter - oldCenter;
    
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        controlPoints[i] += delta;
        targetPoints[i] += delta;
    }
    
    if (oldSize.isValid()) {
        QVector2D resizeForce(
            (newSize.width() - oldSize.width()) * 0.05,
            (newSize.height() - oldSize.height()) * 0.05
        );
        
        applyForce(resizeForce, velocity, blobCenter, controlPoints, oldSize.width() / 2.0);
    }
}

void ResizingState::applyForce(const QVector2D& force,
                             std::vector<QPointF>& velocity,
                             QPointF& blobCenter,
                             const std::vector<QPointF>& controlPoints,
                             double blobRadius) {
    
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        QPointF vectorFromCenter = controlPoints[i] - blobCenter;
        double distanceFromCenter = QVector2D(vectorFromCenter).length();
        
        double forceScale = distanceFromCenter / blobRadius;
        
        if (forceScale > 1.0) forceScale = 1.0;
        
        velocity[i] += QPointF(
            force.x() * forceScale * 0.8,
            force.y() * forceScale * 0.8
        );
    }
    
    blobCenter += QPointF(force.x() * 0.2, force.y() * 0.2);
}