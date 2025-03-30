#ifndef BLOBRENDERER_H
#define BLOBRENDERER_H

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QPointF>
#include <QRandomGenerator>
#include <QTimer>
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
    BlobRenderer() {
        m_glitchTimer = new QTimer();
        m_glitchTimer->setInterval(3000 + QRandomGenerator::global()->bounded(4000));
        QObject::connect(m_glitchTimer, &QTimer::timeout, [this]() {
            if (QRandomGenerator::global()->bounded(100) < 40) {
                // 40% szans na glitch
                triggerGlitch(0.2 + QRandomGenerator::global()->bounded(40) / 100.0);
            }

            // Ustaw następny interwał
            m_glitchTimer->setInterval(3000 + QRandomGenerator::global()->bounded(4000));
        });
        m_glitchTimer->start();
    }

    void renderBlob(QPainter &painter,
                    const std::vector<QPointF> &controlPoints,
                    const QPointF &blobCenter,
                    const BlobConfig::BlobParameters &params,
                    int width, int height);

    void resetGridBuffer() {
        m_gridBuffer = QPixmap();
    }

    void drawBackground(QPainter &painter,
                        const QColor &backgroundColor,
                        const QColor &gridColor,
                        int gridSpacing,
                        int width, int height);

    // Nowa metoda do renderowania całej sceny
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

    void drawHUD(QPainter &painter, const QPointF &blobCenter, double blobRadius, const QColor &hudColor, int width,
                 int height);

    void triggerGlitch(double intensity = 0.5) {
        m_glitchIntensity = intensity;

        // Za 300-700ms zmniejsz intensywność
        QTimer::singleShot(300 + QRandomGenerator::global()->bounded(400),
                           [this]() { m_glitchIntensity *= 0.3; });

        // Po 1s całkowicie wyłącz
        QTimer::singleShot(1000, [this]() { m_glitchIntensity = 0.0; });
    }

private:
    QPixmap m_gridBuffer;
    QColor m_lastBgColor;
    QColor m_lastGridColor;
    int m_lastGridSpacing = 0;
    QSize m_lastSize;
    double m_glitchIntensity = 0.0;
    QTimer *m_glitchTimer = nullptr;

    QPixmap m_staticBackgroundTexture;
    bool m_staticBackgroundInitialized = false;

    void updateGridBuffer(const QColor &backgroundColor,
                          const QColor &gridColor,
                          int gridSpacing,
                          int width, int height);

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
                     const QColor &borderColor);
};

#endif // BLOBRENDERER_H
