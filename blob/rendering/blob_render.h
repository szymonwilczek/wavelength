#ifndef BLOBRENDERER_H
#define BLOBRENDERER_H

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QPointF>
#include <vector>
#include "../core/blob_config.h"
#include "../utils/blob_path.h"

class BlobRenderer {
public:
    BlobRenderer();
    
    void renderBlob(QPainter& painter,
                   const std::vector<QPointF>& controlPoints,
                   const QPointF& blobCenter,
                   const BlobConfig::BlobParameters& params,
                   int width, int height);
    
private:
    void drawBackground(QPainter& painter,
                      const QColor& backgroundColor,
                      const QColor& gridColor,
                      int gridSpacing,
                      int width, int height);
    
    void drawGlowEffect(QPainter& painter,
                       const QPainterPath& blobPath,
                       const QColor& borderColor,
                       int glowRadius);
    
    void drawBorder(QPainter& painter,
                   const QPainterPath& blobPath,
                   const QColor& borderColor,
                   int borderWidth);
    
    void drawFilling(QPainter& painter,
                    const QPainterPath& blobPath,
                    const QPointF& blobCenter,
                    double blobRadius,
                    const QColor& borderColor);
};

#endif // BLOBRENDERER_H