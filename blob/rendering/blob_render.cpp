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
    

    QPainterPath blobPath = BlobPath::createBlobPath(controlPoints, controlPoints.size());
    
    drawGlowEffect(painter, blobPath, params.borderColor, params.glowRadius);
    
    drawBorder(painter, blobPath, params.borderColor, params.borderWidth);
    
    drawFilling(painter, blobPath, blobCenter, params.blobRadius, params.borderColor);
}

void BlobRenderer::updateGridBuffer(const QColor& backgroundColor,
                                  const QColor& gridColor,
                                  int gridSpacing,
                                  int width, int height) {
    m_gridBuffer = QPixmap(width, height);

    QPainter bufferPainter(&m_gridBuffer);
    bufferPainter.setRenderHint(QPainter::Antialiasing, false); 

    bufferPainter.fillRect(QRect(0, 0, width, height), backgroundColor);

    bufferPainter.setPen(QPen(gridColor, 1, Qt::SolidLine));

    for (int y = 0; y < height; y += gridSpacing) {
        bufferPainter.drawLine(0, y, width, y);
    }

    for (int x = 0; x < width; x += gridSpacing) {
        bufferPainter.drawLine(x, 0, x, height);
    }

    m_lastBgColor = backgroundColor;
    m_lastGridColor = gridColor;
    m_lastGridSpacing = gridSpacing;
    m_lastSize = QSize(width, height);
}

void BlobRenderer::drawBackground(QPainter& painter,
                                const QColor& backgroundColor,
                                const QColor& gridColor,
                                int gridSpacing,
                                int width, int height) {

    bool needsUpdate = m_gridBuffer.isNull() ||
                      backgroundColor != m_lastBgColor ||
                      gridColor != m_lastGridColor ||
                      gridSpacing != m_lastGridSpacing ||
                      QSize(width, height) != m_lastSize;

    if (needsUpdate) {
        updateGridBuffer(backgroundColor, gridColor, gridSpacing, width, height);
    }

    painter.drawPixmap(0, 0, m_gridBuffer);
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
    centerColor.setAlpha(10);
    gradient.setColorAt(0, centerColor);
    gradient.setColorAt(1, QColor(0, 0, 0, 0));
    
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawPath(blobPath);
}

void BlobRenderer::renderScene(QPainter& painter,
                             const std::vector<QPointF>& controlPoints,
                             const QPointF& blobCenter,
                             const BlobConfig::BlobParameters& params,
                             const BlobRenderState& renderState,
                             int width, int height,
                             QPixmap& backgroundCache,
                             QSize& lastBackgroundSize,
                             QColor& lastBgColor,
                             QColor& lastGridColor,
                             int& lastGridSpacing) {

    if (renderState.animationState == BlobConfig::MOVING || renderState.animationState == BlobConfig::RESIZING) {
        painter.setRenderHint(QPainter::Antialiasing, false);
    } else {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    }

    // Sprawdź czy potrzebujemy zaktualizować bufor tła
    bool shouldUpdateBackgroundCache =
        backgroundCache.isNull() ||
        lastBackgroundSize != QSize(width, height) ||
        lastBgColor != params.backgroundColor ||
        lastGridColor != params.gridColor ||
        lastGridSpacing != params.gridSpacing;

    // W stanie IDLE i gdy nie ma absorpcji, używamy bufora tła
    if (renderState.animationState == BlobConfig::IDLE &&
        !renderState.isBeingAbsorbed &&
        !renderState.isAbsorbing) {

        if (shouldUpdateBackgroundCache) {
            backgroundCache = QPixmap(width, height);
            backgroundCache.fill(Qt::transparent);

            QPainter cachePainter(&backgroundCache);
            cachePainter.setRenderHint(QPainter::Antialiasing, true);
            drawBackground(cachePainter, params.backgroundColor,
                           params.gridColor, params.gridSpacing,
                           width, height);

            lastBackgroundSize = QSize(width, height);
            lastBgColor = params.backgroundColor;
            lastGridColor = params.gridColor;
            lastGridSpacing = params.gridSpacing;
        }

        painter.drawPixmap(0, 0, backgroundCache);
    } else {
        painter.setRenderHint(QPainter::Antialiasing, renderState.animationState == BlobConfig::IDLE);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, renderState.animationState == BlobConfig::IDLE);

        drawBackground(painter, params.backgroundColor,
                      params.gridColor, params.gridSpacing,
                      width, height);
    }

    // Obsługa stanów absorpcji
    if (renderState.isBeingAbsorbed || renderState.isClosingAfterAbsorption) {
        painter.setOpacity(renderState.opacity);
        painter.save();
        QPointF center = blobCenter;
        painter.translate(center);
        painter.scale(renderState.scale, renderState.scale);
        painter.translate(-center);
    }
    else if (renderState.isAbsorbing) {
        painter.save();
        QPointF center = blobCenter;

        if (renderState.isPulseActive) {
            QPen extraGlowPen(params.borderColor.lighter(150));
            extraGlowPen.setWidth(params.borderWidth + 10);
            painter.setPen(extraGlowPen);

            QPainterPath blobPath = BlobPath::createBlobPath(controlPoints, controlPoints.size());
            painter.drawPath(blobPath);
        }

        painter.translate(center);
        painter.scale(renderState.scale, renderState.scale);
        painter.translate(-center);
    }

    // Renderuj blob
    renderBlob(painter, controlPoints, blobCenter, params, width, height);

    // Przywróć poprzedni stan transformacji
    if (renderState.isBeingAbsorbed || renderState.isAbsorbing) {
        painter.restore();
    }
}