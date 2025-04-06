#ifndef BLOB_RENDERER_H
#define BLOB_RENDERER_H

#include <QPainter>
#include <QPixmap>
#include <QRandomGenerator>
#include <vector>

#include "path_markers_manager.h"
#include "../core/blob_config.h"

struct BlobRenderState {
    bool isBeingAbsorbed = false;
    bool isAbsorbing = false;
    bool isClosingAfterAbsorption = false;
    bool isPulseActive = false;
    double opacity = 1.0;
    double scale = 1.0;
    BlobConfig::AnimationState animationState = BlobConfig::IDLE;
};

class BlobRenderer {
public:


    BlobRenderer() :
    m_staticBackgroundInitialized(false),
    m_glitchIntensity(0),
    m_lastUpdateTime(0),
    m_idleHudInitialized(false),
    m_isRenderingActive(true),
    m_lastAnimationState(BlobConfig::MOVING) {}

    void renderBlob(QPainter &painter,
                    const std::vector<QPointF> &controlPoints,
                    const QPointF &blobCenter,
                    const BlobConfig::BlobParameters &params,
                    int width, int height,
                    BlobConfig::AnimationState animationState);

    void updateGridBuffer(const QColor &backgroundColor,
                          const QColor &gridColor,
                          int gridSpacing,
                          int width, int height);

    void drawBackground(QPainter &painter,
                        const QColor &backgroundColor,
                        const QColor &gridColor,
                        int gridSpacing,
                        int width, int height);

    void renderScene(QPainter &painter,
                     const std::vector<QPointF> &controlPoints,
                     const QPointF &blobCenter,
                     const BlobConfig::BlobParameters &params,
                     const BlobRenderState &renderState,
                     int width, int height,
                     QPixmap &backgroundCache,
                     QSize &lastBackgroundSize,
                     QColor &lastBgColor,
                     QColor &lastGridColor,
                     int &lastGridSpacing);
    void initializeIdleState(const QPointF& blobCenter, double blobRadius, const QColor& hudColor, int width,
                             int height);

    void drawCompleteHUD(QPainter& painter, const QPointF& blobCenter, double blobRadius, const QColor& hudColor,
                         int width,
                         int height);
    void drawStaticHUD(QPainter& painter, const QPointF& blobCenter, double blobRadius, const QColor& hudColor,
                       int width,
                       int height);

    void resetGridBuffer() {
        m_gridBuffer = QPixmap();
    }

    void setGlitchIntensity(double intensity) {
        m_glitchIntensity = intensity;
    }

private:
    QPixmap m_staticBackgroundTexture;
    bool m_staticBackgroundInitialized;
    QPixmap m_gridBuffer;
    QColor m_lastBgColor;
    QColor m_lastGridColor;
    int m_lastGridSpacing;
    QSize m_lastSize;
    double m_glitchIntensity;

    // Nowy obiekt zarządzający markerami
    PathMarkersManager m_markersManager;
    qint64 m_lastUpdateTime;

    QString m_idleBlobId;
    double m_idleAmplitude;
    QString m_idleTimestamp;
    bool m_idleHudInitialized;
    QPixmap m_completeHudBuffer;

    // Flaga wskazująca czy renderowanie jest aktywne
    bool m_isRenderingActive;
    BlobConfig::AnimationState m_lastAnimationState;

    QPixmap m_staticHudBuffer;  // Tylko dla elementów HUD

    void drawGlowEffect(QPainter &painter,
                       const QPainterPath &blobPath,
                       const QColor &borderColor,
                       int glowRadius);

    void drawBorder(QPainter &painter,
                   const QPainterPath &blobPath,
                   const QColor &borderColor,
                   int borderWidth);

    void drawFilling(QPainter &painter,
                   const QPainterPath &blobPath,
                   const QPointF &blobCenter,
                   double blobRadius,
                   const QColor &borderColor,
                   BlobConfig::AnimationState animationState);

    void drawDynamicHUD(QPainter& painter, const QPointF& blobCenter, double blobRadius, const QColor& hudColor,
                        int width,
                        int height);
    void drawHUD(QPainter &painter,
                 const QPointF &blobCenter,
                 double blobRadius,
                 const QColor &hudColor,
                 int width, int height);
};

#endif // BLOB_RENDERER_H