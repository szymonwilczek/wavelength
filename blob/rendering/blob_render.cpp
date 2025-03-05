#include "blob_render.h"
#include <QRadialGradient>

BlobRenderer::BlobRenderer() {
}

void BlobRenderer::renderBlob(QPainter& painter, 
                            const std::vector<QPointF>& controlPoints,
                            const QPointF& blobCenter,
                            const BlobConfig::BlobParameters& params,
                            int width, int height) {
    
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    drawBackground(painter, params.backgroundColor, params.gridColor,
                 params.gridSpacing, width, height);
    
    QPainterPath blobPath = BlobPath::createBlobPath(controlPoints, controlPoints.size());
    
    drawGlowEffect(painter, blobPath, params.borderColor, params.glowRadius);
    
    drawBorder(painter, blobPath, params.borderColor, params.borderWidth);
    
    drawFilling(painter, blobPath, blobCenter, params.blobRadius, params.borderColor);
}

void BlobRenderer::drawBackground(QPainter& painter, 
                                const QColor& backgroundColor,
                                const QColor& gridColor,
                                int gridSpacing,
                                int width, int height) {
    
    painter.fillRect(QRect(0, 0, width, height), backgroundColor);
    
    painter.setPen(QPen(gridColor, 1, Qt::SolidLine));
    
    for (int y = 0; y < height; y += gridSpacing) {
        painter.drawLine(0, y, width, y);
    }
    
    for (int x = 0; x < width; x += gridSpacing) {
        painter.drawLine(x, 0, x, height);
    }
}

void BlobRenderer::drawGlowEffect(QPainter& painter, 
                                const QPainterPath& blobPath,
                                const QColor& borderColor,
                                int glowRadius) {
    
    QColor glowColor = borderColor;
    glowColor.setAlpha(80);
    
    for (int i = glowRadius; i > 0; i -= 2) {
        QPen glowPen(glowColor);
        glowPen.setWidth(i);
        painter.setPen(glowPen);
        painter.drawPath(blobPath);
        
        glowColor.setAlpha(glowColor.alpha() - 15);
    }
}

void BlobRenderer::drawBorder(QPainter& painter, 
                            const QPainterPath& blobPath,
                            const QColor& borderColor,
                            int borderWidth) {
    
    QPen borderPen(borderColor);
    borderPen.setWidth(borderWidth);
    painter.setPen(borderPen);
    painter.drawPath(blobPath);
}

void BlobRenderer::drawFilling(QPainter& painter, 
                             const QPainterPath& blobPath,
                             const QPointF& blobCenter,
                             double blobRadius,
                             const QColor& borderColor) {
    
    QRadialGradient gradient(blobCenter, blobRadius);
    
    QColor centerColor = borderColor;
    centerColor.setAlpha(30);
    gradient.setColorAt(0, centerColor);
    gradient.setColorAt(1, QColor(0, 0, 0, 0));
    
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawPath(blobPath);
}