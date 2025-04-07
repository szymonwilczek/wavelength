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
        QColor mainGridColor = QColor(0, 195, 255, 40);
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
    bool bufferNeedsUpdate = m_glowBuffer.isNull() ||
                            blobPath != m_lastGlowPath ||
                            borderColor != m_lastGlowColor ||
                            glowRadius != m_lastGlowRadius ||
                            viewportSize != m_lastGlowSize ||
                            m_lastAnimationState != BlobConfig::IDLE;

    // Użyj zbuforowanego efektu glow w stanie IDLE jeśli to możliwe
    if (m_lastAnimationState == BlobConfig::IDLE && !bufferNeedsUpdate) {
        // Rysuj zbuforowany efekt glow
        painter.drawPixmap(0, 0, m_glowBuffer);
        return;
    }

    // Przygotuj nowy bufor przed resetowaniem starego
    QPixmap newGlowBuffer;

    // Musimy zaktualizować bufor lub jesteśmy w stanie animacji
    if (m_lastAnimationState == BlobConfig::IDLE && bufferNeedsUpdate) {
        // Tworzymy nowy bufor tylko dla stanu IDLE
        newGlowBuffer = QPixmap(viewportSize);
        newGlowBuffer.fill(Qt::transparent);

        QPainter bufferPainter(&newGlowBuffer);
        bufferPainter.setRenderHint(QPainter::Antialiasing, true);

        // Rysujemy efekt glow do bufora
        renderGlowEffect(bufferPainter, blobPath, borderColor, glowRadius);
        bufferPainter.end();

        // Podmień bufory dopiero po zakończeniu rysowania
        m_glowBuffer = newGlowBuffer;

        // Zapisz parametry dla późniejszego porównania
        m_lastGlowPath = blobPath;
        m_lastGlowColor = borderColor;
        m_lastGlowRadius = glowRadius;
        m_lastGlowSize = viewportSize;

        // Użyj nowego zbuforowanego efektu
        painter.drawPixmap(0, 0, m_glowBuffer);
    } else {
        renderGlowEffect(painter, blobPath, borderColor, glowRadius);
    }
}

void BlobRenderer::renderGlowEffect(QPainter &painter,
                                   const QPainterPath &blobPath,
                                   const QColor &borderColor,
                                   int glowRadius) {
    // Zapisz stan malarza
    painter.save();

    // 1. Rysujemy główny efekt poświaty (zewnętrzny glow)
    QColor outerGlowColor = borderColor;
    outerGlowColor.setAlpha(50);

    // Używamy gradient radialny dla lepszego efektu zanikania
    for (int i = glowRadius; i > 0; i -= 2) {
        // Stopniowo zmniejszamy nieprzezroczystość
        int alpha = qMax(5, 50 - (glowRadius - i) * 3);
        outerGlowColor.setAlpha(alpha);

        QPen glowPen(outerGlowColor);
        glowPen.setWidth(i);
        glowPen.setCapStyle(Qt::RoundCap);  // Zaokrąglone końce
        glowPen.setJoinStyle(Qt::RoundJoin);  // Zaokrąglone połączenia
        painter.setPen(glowPen);
        painter.drawPath(blobPath);
    }

    // 2. Dodajemy drugą warstwę poświaty (bardziej intensywna, bliżej krawędzi)
    QColor midGlowColor = borderColor.lighter(120);
    midGlowColor.setAlpha(80);

    for (int i = glowRadius/2; i > 0; i -= 1) {
        int alpha = qMin(80, 40 + i * 4);
        midGlowColor.setAlpha(alpha);

        QPen midGlowPen(midGlowColor);
        midGlowPen.setWidth(i);
        midGlowPen.setCapStyle(Qt::RoundCap);
        midGlowPen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(midGlowPen);
        painter.drawPath(blobPath);
    }

    // 3. Dodajemy jasny wewnętrzny blask (efekt jarzenia się)
    QColor innerGlowColor = borderColor.lighter(160);
    innerGlowColor.setAlpha(100);

    QPen innerGlowPen(innerGlowColor);
    innerGlowPen.setWidth(2);
    painter.setPen(innerGlowPen);
    painter.drawPath(blobPath);

    // 4. Dodajemy bardzo intensywne punktowe "iskrzenie" wzdłuż ścieżki
    QColor sparkColor = borderColor.lighter(200);
    sparkColor.setAlpha(180);

    QPen sparkPen(sparkColor);
    sparkPen.setWidth(1);
    painter.setPen(sparkPen);
    painter.drawPath(blobPath);

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