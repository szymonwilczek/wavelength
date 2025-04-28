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

    drawFilling(painter, blobPath, blobCenter, params.blobRadius, params.borderColor, animationState);  // Przekazujemy stan animacji
}

void BlobRenderer::updateGridBuffer(const QColor &backgroundColor,
                                    const QColor &gridColor,
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
        const QColor& mainGridColor = gridColor;
        bufferPainter.setPen(QPen(mainGridColor, 1, Qt::SolidLine));

        // Główne linie siatki
        for (int y = 0; y < height; y += gridSpacing) {
            bufferPainter.drawLine(0, y, width, y);
        }

        for (int x = 0; x < width; x += gridSpacing) {
            bufferPainter.drawLine(x, 0, x, height);
        }

        // Dodajemy podsiatkę o mniejszej intensywności
        QColor subgridColor = QColor(gridColor.redF(), gridColor.greenF(), gridColor.blueF(), 0.35);
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
    QSize viewportSize = QSize(painter.device()->width(), painter.device()->height());

    // Sprawdzamy, czy możemy użyć istniejącego bufora
    bool bufferNeedsUpdate = m_glowBuffer.isNull() ||
                            blobPath != m_lastGlowPath ||
                            borderColor != m_lastGlowColor ||
                            glowRadius != m_lastGlowRadius ||
                            viewportSize != m_lastGlowSize;

    // W stanie IDLE zawsze używaj zbuforowanego efektu jeśli to możliwe
    if (m_lastAnimationState == BlobConfig::IDLE && !bufferNeedsUpdate && !m_glowBuffer.isNull()) {
        painter.drawPixmap(0, 0, m_glowBuffer);
        return;
    }

    // W stanie animacji buforuj efekt co kilka klatek dla lepszej wydajności
    static int frameCounter = 0;
    bool shouldUpdateBuffer = bufferNeedsUpdate ||
                             (m_lastAnimationState != BlobConfig::IDLE &&
                              frameCounter++ % 5 == 0);  // Aktualizuj co 5 klatek

    if (shouldUpdateBuffer) {
        // Tworzymy nowy bufor
        QPixmap newGlowBuffer(viewportSize);
        newGlowBuffer.fill(Qt::transparent);

        QPainter bufferPainter(&newGlowBuffer);
        bufferPainter.setRenderHint(QPainter::Antialiasing, true);

        // Rysujemy zoptymalizowany efekt glow do bufora
        renderGlowEffect(bufferPainter, blobPath, borderColor, glowRadius);
        bufferPainter.end();

        // Podmień bufory dopiero po zakończeniu rysowania
        m_glowBuffer = newGlowBuffer;

        // Zapisz parametry dla późniejszego porównania
        m_lastGlowPath = blobPath;
        m_lastGlowColor = borderColor;
        m_lastGlowRadius = glowRadius;
        m_lastGlowSize = viewportSize;
    }

    // Zawsze używaj zbuforowanego efektu (aktualnego lub poprzedniego)
    painter.drawPixmap(0, 0, m_glowBuffer);
}

void BlobRenderer::renderGlowEffect(QPainter &painter,
                                   const QPainterPath &blobPath,
                                   const QColor &borderColor,
                                   int glowRadius) {
    // Zapisz stan malarza
    painter.save();

    // Używamy kompozycji SourceOver dla najbardziej realistycznych efektów
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // OPTYMALIZACJA: Rysujemy tylko 3 warstwy zamiast wielu pętli

    // 1. Warstwa podstawowa (łagodna poświata) - najbardziej zewnętrzna
    {
        QColor outerColor = borderColor;
        // Zmniejszamy nasycenie dla bardziej realistycznego efektu
        outerColor = QColor::fromHslF(
            outerColor.hslHueF(),
            qMin(0.9, outerColor.hslSaturationF() * 0.7),
            outerColor.lightnessF(),
            0.2  // Niska nieprzezroczystość dla łagodnego efektu
        );

        QPen outerPen(outerColor);
        outerPen.setWidth(glowRadius);
        outerPen.setCapStyle(Qt::RoundCap);
        outerPen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(outerPen);
        painter.drawPath(blobPath);
    }

    // 2. Warstwa środkowa (intensywny blask) - typowa dla neonów
    {
        QColor midColor = borderColor.lighter(115);
        // Neony mają charakterystyczny blask pośredni
        midColor = QColor::fromHslF(
            midColor.hslHueF(),
            qMin(1.0, midColor.hslSaturationF() * 1.1),
            qMin(0.9, midColor.lightnessF() * 1.2),
            0.6  // Wyższa nieprzezroczystość dla intensywnego blasku
        );

        QPen midPen(midColor);
        midPen.setWidth(glowRadius/2);
        midPen.setCapStyle(Qt::RoundCap);
        midPen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(midPen);
        painter.drawPath(blobPath);
    }

    // 3. Warstwa wewnętrzna (jasne jądro) - charakterystyczna dla neonówek
    {
        // Prawie biały kolor w środku - typowe dla neonów
        QColor coreColor = borderColor.lighter(160);
        coreColor = QColor::fromHslF(
            coreColor.hslHueF(),
            qMin(0.3, coreColor.hslSaturationF() * 0.5), // Niższe nasycenie
            0.9, // Wysokie rozjaśnienie - charakterystyczne dla neonów
            0.95 // Wysoka nieprzezroczystość
        );

        QPen corePen(coreColor);
        corePen.setWidth(3); // Stały wąski rdzeń
        corePen.setCapStyle(Qt::RoundCap);
        corePen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(corePen);
        painter.drawPath(blobPath);
    }

    // Przywróć stan malarza
    painter.restore();
}

void BlobRenderer::drawBorder(QPainter &painter,
                              const QPainterPath &blobPath,
                              const QColor &borderColor,
                              int borderWidth) {

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
    // Wykrywamy, czy będzie zmiana stanu
    bool stateChangingToIdle = (renderState.animationState == BlobConfig::IDLE && m_lastAnimationState != BlobConfig::IDLE);

    // Wykrycie przejścia do stanu IDLE - PRZYGOTUJ BUFORY PRZED ZMIANĄ STANU
    if (stateChangingToIdle) {
        // Najpierw inicjalizujemy wartości dla stanu IDLE
        initializeIdleState(blobCenter, params.blobRadius, params.borderColor, width, height);

        // Przygotowujemy bufor HUD przed resetowaniem flag
        if (!m_idleHudInitialized) {
            m_staticHudBuffer = QPixmap(width, height);
            m_staticHudBuffer.fill(Qt::transparent);

            QPainter hudPainter(&m_staticHudBuffer);
            hudPainter.setRenderHint(QPainter::Antialiasing, true);
            drawCompleteHUD(hudPainter, blobCenter, params.blobRadius, params.borderColor, width, height);
            hudPainter.end();

            m_idleHudInitialized = true;
        }
    }

    // Po przygotowaniu buforów możemy aktualizować stan
    if (stateChangingToIdle) {
        m_isRenderingActive = false;
    } else if (renderState.animationState != BlobConfig::IDLE) {
        m_isRenderingActive = true;
        // Nie resetuj od razu flagi - poczekaj aż nowe bufory będą gotowe
        // m_idleHudInitialized = false;
    }

    // Dopiero po przygotowaniu nowych buforów aktualizujemy stan
    m_lastAnimationState = renderState.animationState;

    // Sprawdź czy potrzebujemy zaktualizować bufor tła - POZOSTAŁA CZĘŚĆ BEZ ZMIAN
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

        // Używamy przygotowanego wcześniej HUD
        if (m_idleHudInitialized && !m_staticHudBuffer.isNull()) {
            painter.drawPixmap(0, 0, m_staticHudBuffer);
        }
    }
    else {
        // Standardowe renderowanie dla aktywnych stanów
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Rysuj dynamiczne tło
        drawBackground(painter, params.backgroundColor,
                       params.gridColor, params.gridSpacing,
                       width, height);

        // Renderuj blob
        renderBlob(painter, controlPoints, blobCenter, params, width, height, renderState.animationState);

        // Po wyjściu ze stanu IDLE możemy zresetować flagi buforów
        if (m_lastAnimationState == BlobConfig::IDLE && renderState.animationState != BlobConfig::IDLE) {
            m_idleHudInitialized = false;
        }
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

void BlobRenderer::forceHUDInitialization(const QPointF &blobCenter, double blobRadius, const QColor &hudColor,
    int width, int height) {
    initializeIdleState(blobCenter, blobRadius, hudColor, width, height);

    m_staticHudBuffer = QPixmap(width, height);
    m_staticHudBuffer.fill(Qt::transparent);

    QPainter hudPainter(&m_staticHudBuffer);
    hudPainter.setRenderHint(QPainter::Antialiasing, true);
    drawCompleteHUD(hudPainter, blobCenter, blobRadius, hudColor, width, height);
    hudPainter.end();

    m_idleHudInitialized = true;
}
