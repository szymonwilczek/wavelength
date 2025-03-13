#ifndef BLOBRENDERER_H
#define BLOBRENDERER_H

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QPointF>
#include <vector>
#include "../core/blob_config.h"
#include "../utils/blob_path.h"

// Struktura do przechowywania stanu wyświetlania bloba
struct BlobRenderState {
    bool isBeingAbsorbed = false;
    bool isAbsorbing = false;
    bool isClosingAfterAbsorption = false;
    bool isPulseActive = false;
    double opacity = 1.0;
    double scale = 1.0;
    BlobConfig::AnimationState animationState = BlobConfig::IDLE;
    bool isInTransitionToIdle = false;
};

class BlobRenderer {
public:
    BlobRenderer();

    void renderBlob(QPainter& painter,
                   const std::vector<QPointF>& controlPoints,
                   const QPointF& blobCenter,
                   const BlobConfig::BlobParameters& params,
                   int width, int height);

    void resetGridBuffer() {
        m_gridBuffer = QPixmap();
    }

    void drawBackground(QPainter& painter,
                         const QColor& backgroundColor,
                         const QColor& gridColor,
                         int gridSpacing,
                         int width, int height);

    // Nowa metoda do renderowania całej sceny
    void renderScene(QPainter& painter,
                    const std::vector<QPointF>& controlPoints,
                    const QPointF& blobCenter,
                    const BlobConfig::BlobParameters& params,
                    const BlobRenderState& renderState,
                    int width, int height,
                    QPixmap& backgroundCache,
                    QSize& lastBackgroundSize,
                    QColor& lastBgColor,
                    QColor& lastGridColor,
                    int& lastGridSpacing);

private:
    QPixmap m_gridBuffer;
    QColor m_lastBgColor;
    QColor m_lastGridColor;
    int m_lastGridSpacing = 0;
    QSize m_lastSize;

    void updateGridBuffer(const QColor& backgroundColor,
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