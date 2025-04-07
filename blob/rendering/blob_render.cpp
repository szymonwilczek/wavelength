#include "blob_render.h"

#include <QDateTime>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QDebug>

#include "../utils/blob_path.h"

void BlobRenderer::renderBlob(QPainter &painter,
                              const std::vector<QPointF> &controlPoints,
                              const QPointF &blobCenter,
                              const BlobConfig::BlobParameters &params,
                              int width, int height,
                              BlobConfig::AnimationState animationState) {  // Dodany parametr
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath blobPath = BlobPath::createBlobPath(controlPoints, controlPoints.size());

    drawGlowEffect(painter, blobPath, params.borderColor, params.glowRadius);

    drawBorder(painter, blobPath, params.borderColor, params.borderWidth);

    // drawFilling(painter, blobPath, blobCenter, params.blobRadius, params.borderColor, animationState);  // Przekazujemy stan animacji
}

void BlobRenderer::updateGridBuffer(const QColor &backgroundColor,
                                    const QColor &gridColor,
                                    int gridSpacing,
                                    int width, int height) {
    qDebug() << "UPDATE GRID BUFFER";
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

void BlobRenderer::drawBackground(QPainter &painter,
                                  const QColor &backgroundColor,
                                  const QColor &gridColor,
                                  int gridSpacing,
                                  int width, int height) {
    bool needsGridUpdate = m_gridBuffer.isNull() ||
                           backgroundColor != m_lastBgColor ||
                           gridColor != m_lastGridColor ||
                           gridSpacing != m_lastGridSpacing ||
                           QSize(width, height) != m_lastSize;

    qDebug() << "DRAW BACKGROUND";

    // Tworzymy statyczną teksturę tła tylko raz (z neonowymi punktami)
    if (!m_staticBackgroundInitialized) {
        // Stała wielkość tekstury bazowej (niezależnie od rozmiaru okna)
        const int textureSize = 800;
        m_staticBackgroundTexture = QPixmap(textureSize, textureSize);
        m_staticBackgroundTexture.fill(Qt::transparent);

        QPainter texturePainter(&m_staticBackgroundTexture);
        texturePainter.setRenderHint(QPainter::Antialiasing, true);

        // Ciemniejsze tło z gradientem
        QLinearGradient bgGradient(0, 0, textureSize, textureSize);
        bgGradient.setColorAt(0, QColor(0, 15, 30));
        bgGradient.setColorAt(1, QColor(10, 25, 40));
        texturePainter.fillRect(QRect(0, 0, textureSize, textureSize), bgGradient);

        m_staticBackgroundInitialized = true;
    }

    // Najpierw rysujemy statyczne tło z punktami
    painter.drawPixmap(QRect(0, 0, width, height), m_staticBackgroundTexture);

    // Następnie rysujemy tylko siatkę (bez punktów) jeśli potrzeba aktualizacji
    if (needsGridUpdate) {
        m_gridBuffer = QPixmap(width, height);
        m_gridBuffer.fill(Qt::transparent);

        QPainter bufferPainter(&m_gridBuffer);


        // Rysowanie głównej siatki
        QColor mainGridColor = QColor(0, 195, 255, 40);
        qDebug() << "Grid color:" << mainGridColor;
        bufferPainter.setPen(QPen(mainGridColor, 1, Qt::SolidLine));

        // Główne linie siatki
        for (int y = 0; y < height; y += gridSpacing) {
            bufferPainter.drawLine(0, y, width, y);
        }

        for (int x = 0; x < width; x += gridSpacing) {
            bufferPainter.drawLine(x, 0, x, height);
        }

        // Dodajemy podsiatkę o mniejszej intensywności
        QColor subgridColor = QColor(0,130, 200, 35);
        qDebug() << "Subgrid color:" << subgridColor;
        subgridColor.setAlphaF(0.3);
        bufferPainter.setPen(QPen(subgridColor, 0.5, Qt::DotLine));

        int subGridSpacing = gridSpacing / 2;
        for (int y = subGridSpacing; y < height; y += gridSpacing) {
            bufferPainter.drawLine(0, y, width, y);
        }

        for (int x = subGridSpacing; x < width; x += gridSpacing) {
            bufferPainter.drawLine(x, 0, x, height);
        }

        m_lastBgColor = backgroundColor;
        m_lastGridColor = gridColor;
        m_lastGridSpacing = gridSpacing;
        m_lastSize = QSize(width, height);
    }

    // Rysuj siatkę na wierzchu statycznego tła
    painter.drawPixmap(0, 0, m_gridBuffer);
}

void BlobRenderer::drawGlowEffect(QPainter &painter,
                                  const QPainterPath &blobPath,
                                  const QColor &borderColor,
                                  int glowRadius) {

    qDebug() << "GLOW EFFECT";
    // Intensywniejszy efekt poświaty
    QColor glowColor = borderColor;
    glowColor.setAlpha(100);

    // Zewnętrzna poświata
    for (int i = glowRadius; i > 0; i -= 1) {
        QPen glowPen(glowColor);
        glowPen.setWidth(i);
        painter.setPen(glowPen);
        painter.drawPath(blobPath);

        glowColor.setAlpha(glowColor.alpha() - 8);
    }

    // Dodajmy także wewnętrzną poświatę
    QColor innerGlow = borderColor.lighter(150);
    innerGlow.setAlpha(60);

    QPainterPath innerPath = blobPath;
    QPen innerGlowPen(innerGlow);
    innerGlowPen.setWidth(2);
    painter.setPen(innerGlowPen);
    painter.drawPath(innerPath);
}

void BlobRenderer::drawBorder(QPainter &painter,
                              const QPainterPath &blobPath,
                              const QColor &borderColor,
                              int borderWidth) {

    qDebug() << "DRAW BORDER";
    // Główne obramowanie neonowe
    QPen borderPen(borderColor);
    borderPen.setWidth(borderWidth);
    painter.setPen(borderPen);
    painter.drawPath(blobPath);

    // Dodatkowe jaśniejsze obramowanie wewnętrzne
    QPen innerPen(borderColor.lighter(150));
    innerPen.setWidth(1);
    painter.setPen(innerPen);
    painter.drawPath(blobPath);

    // Rysuj markery na ścieżce używając naszego managera
    // qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    // m_markersManager.drawMarkers(painter, blobPath, currentTime);
}


void BlobRenderer::drawFilling(QPainter &painter,
                            const QPainterPath &blobPath,
                            const QPointF &blobCenter,
                            double blobRadius,
                            const QColor &borderColor,
                            BlobConfig::AnimationState animationState) {  // Dodany parametr

    qDebug() << "DRAW FILLING";
    // Cyfrowy gradient z zanikaniem dla bloba
    QRadialGradient gradient(blobCenter, blobRadius);

    // Jaśniejszy środek
    QColor centerColor = borderColor.lighter(130);
    centerColor.setAlpha(30);
    gradient.setColorAt(0, centerColor);

    // Środkowy kolor
    QColor midColor = borderColor;
    midColor.setAlpha(15);
    gradient.setColorAt(0.7, midColor);

    // Półprzezroczysty brzeg
    QColor edgeColor = borderColor.darker(120);
    edgeColor.setAlpha(5);
    gradient.setColorAt(0.9, edgeColor);
    gradient.setColorAt(1, QColor(0, 0, 0, 0));

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawPath(blobPath);
}

void BlobRenderer::renderScene(QPainter &painter,
                               const std::vector<QPointF> &controlPoints,
                               const QPointF &blobCenter,
                               const BlobConfig::BlobParameters &params,
                               const BlobRenderState &renderState,
                               int width, int height,
                               QPixmap &backgroundCache,
                               QSize &lastBackgroundSize,
                               QColor &lastBgColor,
                               QColor &lastGridColor,
                               int &lastGridSpacing) {
    qDebug() << "RENDER SCENE";

    // Wykrycie przejścia do stanu IDLE
    if (renderState.animationState == BlobConfig::IDLE && m_lastAnimationState != BlobConfig::IDLE) {
        // Inicjalizacja wartości dla stanu IDLE
        initializeIdleState(blobCenter, params.blobRadius, params.borderColor, width, height);
        m_isRenderingActive = false;
    } else if (renderState.animationState != BlobConfig::IDLE) {
        // Dla innych stanów zawsze aktywujemy renderowanie
        m_isRenderingActive = true;
        m_idleHudInitialized = false;
    }

    m_lastAnimationState = renderState.animationState;

    // Sprawdź czy potrzebujemy zaktualizować bufor tła
    bool shouldUpdateBackgroundCache =
            backgroundCache.isNull() ||
            lastBackgroundSize != QSize(width, height) ||
            lastBgColor != params.backgroundColor ||
            lastGridColor != params.gridColor ||
            lastGridSpacing != params.gridSpacing;

    // W stanie IDLE używamy buforowanego tła
    if (renderState.animationState == BlobConfig::IDLE) {
        if (shouldUpdateBackgroundCache) {
            // Aktualizacja bufora tła
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

        // Rysujemy buforowane tło
        painter.drawPixmap(0, 0, backgroundCache);

        // Rysujemy aktywny blob - ZAWSZE będzie animowany
        renderBlob(painter, controlPoints, blobCenter, params, width, height, renderState.animationState);

        // Sprawdzamy czy HUD został już zbuforowany
        if (!m_idleHudInitialized) {
            // Tworzymy bufor HUD
            m_staticHudBuffer = QPixmap(width, height);
            m_staticHudBuffer.fill(Qt::transparent);

            QPainter hudPainter(&m_staticHudBuffer);
            hudPainter.setRenderHint(QPainter::Antialiasing, true);
            drawCompleteHUD(hudPainter, blobCenter, params.blobRadius, params.borderColor, width, height);
            hudPainter.end();

            m_idleHudInitialized = true;
        }

        // Rysujemy zbuforowany HUD na wierzchu
        painter.drawPixmap(0, 0, m_staticHudBuffer);
    }
    else {
        // Standardowe renderowanie dla aktywnych stanów
        painter.setRenderHint(QPainter::Antialiasing, renderState.animationState == BlobConfig::IDLE);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, renderState.animationState == BlobConfig::IDLE);

        // Rysuj dynamiczne tło
        drawBackground(painter, params.backgroundColor,
                       params.gridColor, params.gridSpacing,
                       width, height);


        // Renderuj blob
        renderBlob(painter, controlPoints, blobCenter, params, width, height, renderState.animationState);
    }
}

void BlobRenderer::initializeIdleState(const QPointF &blobCenter, double blobRadius,
                                      const QColor &hudColor, int width, int height) {
    // Losujemy wartości tylko raz przy przejściu do stanu IDLE
    m_idleBlobId = QString("BLOB-ID: %1").arg(QRandomGenerator::global()->bounded(1000, 9999));
    m_idleAmplitude = 1.5 + sin(QDateTime::currentMSecsSinceEpoch() * 0.001) * 0.5;
    m_idleTimestamp = QDateTime::currentDateTime().toString("HH:mm:ss");

    qDebug() << "Inicjalizacja stanu IDLE z ID:" << m_idleBlobId << "i amplitudą:" << m_idleAmplitude;

    // Resetujemy bufor HUD
    m_staticHudBuffer = QPixmap();
    m_idleHudInitialized = false;
}


void BlobRenderer::drawCompleteHUD(QPainter &painter, const QPointF &blobCenter,
                             double blobRadius, const QColor &hudColor,
                             int width, int height) {

    qDebug() << "RENDER HUD";

    QColor techColor = hudColor.lighter(120);
    techColor.setAlpha(180);
    painter.setPen(QPen(techColor, 1));
    painter.setFont(QFont("Consolas", 8));

    // Narożniki ekranu (styl AR)
    int cornerSize = 15;

    // Lewy górny
    painter.drawLine(10, 10, 10 + cornerSize, 10);
    painter.drawLine(10, 10, 10, 10 + cornerSize);
    painter.drawText(15, 25, "BLOB MONITOR");

    // Prawy górny - nie rysujemy zegara, będzie rysowany dynamicznie
    painter.drawLine(width - 10 - cornerSize, 10, width - 10, 10);
    painter.drawLine(width - 10, 10, width - 10, 10 + cornerSize);
    // Usuwamy linię z czasem, bo będzie rysowana oddzielnie
    // painter.drawText(width - 80, 25, m_idleTimestamp);

    // Prawy dolny
    painter.drawLine(width - 10 - cornerSize, height - 10, width - 10, height - 10);
    painter.drawLine(width - 10, height - 10, width - 10, height - 10 - cornerSize);
    painter.drawText(width - 70, height - 25, QString("R: %1").arg(int(blobRadius)));

    // Lewy dolny
    painter.drawLine(10, height - 10, 10 + cornerSize, height - 10);
    painter.drawLine(10, height - 10, 10, height - 10 - cornerSize);
    painter.drawText(15, height - 25, QString("AMP: %1").arg(m_idleAmplitude, 0, 'f', 1));  // Ustalona amplituda

    // Okrąg wokół bloba (cel)
    QPen targetPen(techColor, 1, Qt::DotLine);
    painter.setPen(targetPen);
    painter.drawEllipse(blobCenter, blobRadius + 20, blobRadius + 20);

    // Wyświetlamy ustalony ID bloba
    QFontMetrics fm(painter.font());
    int textWidth = fm.horizontalAdvance(m_idleBlobId);
    painter.drawText(blobCenter.x() - textWidth / 2, blobCenter.y() + blobRadius + 30, m_idleBlobId);
}