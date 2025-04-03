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

struct PathMarker {
    double position; // Pozycja na ścieżce (0.0 - 1.0)
    int markerType; // 0-heksagon, 1-trójkąt, 2-okrąg
    int size; // Rozmiar znacznika
    int direction; // 1 = zgodnie z ruchem wskazówek zegara, -1 = przeciwnie
    double colorPhase; // Faza animacji koloru (0.0 - 1.0)
    double colorSpeed; // Prędkość zmiany koloru
};

class BlobRenderer {
public:
    BlobRenderer() = default;

    void renderBlob(QPainter &painter,
                const std::vector<QPointF> &controlPoints,
                const QPointF &blobCenter,
                const BlobConfig::BlobParameters &params,
                int width, int height,
                BlobConfig::AnimationState animationState);  // Dodany parametr



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

    std::vector<PathMarker> m_pathMarkers;
    double m_markerSpeed = 0.1;
    qint64 m_lastUpdateTime = 0;

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

    // Zaktualizuj deklarację metody drawFilling w nagłówku:
    void drawFilling(QPainter &painter,
                     const QPainterPath &blobPath,
                     const QPointF &blobCenter,
                     double blobRadius,
                     const QColor &borderColor,
                     BlobConfig::AnimationState animationState);  // Dodany parametr

    QColor getMarkerColor(int markerType, double colorPhase);
};

#endif // BLOBRENDERER_H
