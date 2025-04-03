#include "blob_render.h"

#include <QDateTime>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QDebug>

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

        // Dodaj losowe punkty świecące (małe neony)
        QColor glowPointColor = QColor(0, 200, 255, 100);
        texturePainter.setPen(Qt::NoPen);

        // Stała liczba punktów dla stałego rozmiaru tekstury
        int numPoints = 200;
        QRandomGenerator *rng = QRandomGenerator::global();
        for (int i = 0; i < numPoints; i++) {
            int x = rng->bounded(textureSize);
            int y = rng->bounded(textureSize);
            int size = rng->bounded(2, 5);

            QRadialGradient pointGlow(x, y, size * 2);
            pointGlow.setColorAt(0, glowPointColor);
            pointGlow.setColorAt(1, QColor(0, 0, 0, 0));

            texturePainter.setBrush(pointGlow);
            texturePainter.drawEllipse(QPointF(x, y), size, size);
        }

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
        QColor mainGridColor = QColor(0, 195, 255, 50);
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

    // Inicjalizacja markerów, jeśli jeszcze nie istnieją
    QRandomGenerator *rng = QRandomGenerator::global();
    if (m_pathMarkers.empty()) {
        int numMarkers = rng->bounded(8, 15); // Liczba markerów
        for (int i = 0; i < numMarkers; i++) {
            PathMarker marker;
            marker.position = rng->bounded(1.0); // Pozycja 0.0-1.0 wzdłuż ścieżki
            marker.markerType = rng->bounded(3); // 0-impuls, 1-fala, 2-kwantowy
            marker.size = 4 + rng->bounded(3); // Rozmiar bazowy znaczników (4-6)
            marker.direction = rng->bounded(2) * 2 - 1; // Losowo 1 lub -1
            marker.colorPhase = rng->bounded(1.0); // Losowa początkowa faza koloru
            marker.colorSpeed = 0.3 + rng->bounded(0.4); // Losowa prędkość zmiany koloru
            marker.tailLength = 0.02 + rng->bounded(0.03); // 2-5% długości ścieżki
            marker.wavePhase = 0.0; // Początkowa faza fali
            marker.quantumOffset = rng->bounded(10.0); // Losowe przesunięcie kwantowe
            m_pathMarkers.push_back(marker);
        }
        m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    }

    // Oblicz długość ścieżki
    double pathLength = 0;
    for (int i = 0; i < blobPath.elementCount() - 1; i++) {
        QPointF p1 = blobPath.elementAt(i);
        QPointF p2 = blobPath.elementAt(i + 1);
        pathLength += QLineF(p1, p2).length();
    }

    // Aktualizacja pozycji markerów
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    double deltaTime = (currentTime - m_lastUpdateTime) / 1000.0; // Czas w sekundach
    m_lastUpdateTime = currentTime;

    // Przesuń markery wzdłuż ścieżki
    for (auto &marker: m_pathMarkers) {
        marker.position += marker.direction * m_markerSpeed * deltaTime;
        if (marker.position > 1.0) {
            marker.position -= 1.0; // Zapętlenie po osiągnięciu końca
        } else if (marker.position < 0.0) {
            marker.position += 1.0; // Zapętlenie po osiągnięciu początku
        }

        // Aktualizacja fazy koloru
        marker.colorPhase += marker.colorSpeed * deltaTime;
        if (marker.colorPhase > 1.0) {
            marker.colorPhase -= 1.0; // Zapętlenie fazy koloru
        }

        // Aktualizacja fazy fali dla efektu zakłóceń
        if (marker.markerType == 1) {
            marker.wavePhase += 1.5 * deltaTime; // Prędkość rozchodzenia się fali
            if (marker.wavePhase > 5.0) {
                marker.wavePhase = 0.0; // Reset fali po osiągnięciu maksymalnego rozmiaru
            }
        }

        // Zapisywanie ścieżki dla impulsu energii
        if (marker.markerType == 0) {
            // Zbieramy punkty dla śladu impulsu
            marker.trailPoints.clear();
        }
    }

    // Rysuj markery w ich aktualnych pozycjach
    for (auto &marker: m_pathMarkers) {
        double pos = marker.position * pathLength;
        double currentLength = 0;
        QPointF markerPos;

        // Znajdź punkt na ścieżce
        for (int j = 0; j < blobPath.elementCount() - 1; j++) {
            QPointF p1 = blobPath.elementAt(j);
            QPointF p2 = blobPath.elementAt(j + 1);
            double segmentLength = QLineF(p1, p2).length();

            if (currentLength + segmentLength >= pos) {
                double t = (pos - currentLength) / segmentLength;
                markerPos = p1 * (1 - t) + p2 * t;

                // Dla impulsów energii, zapisz punkty śladu
                if (marker.markerType == 0) {
                    double tailPos = pos - (marker.tailLength * pathLength);
                    if (tailPos < 0) tailPos += pathLength;

                    double trailCurrentLength = 0;
                    int numTrailPoints = 15; // Liczba punktów w śladzie

                    for (int k = 0; k < numTrailPoints; k++) {
                        double trailT = static_cast<double>(k) / numTrailPoints;
                        double trailPosOnPath = pos - (trailT * marker.tailLength * pathLength);
                        if (trailPosOnPath < 0) trailPosOnPath += pathLength;

                        // Znajdź punkt na ścieżce dla śladu
                        double trailLength = 0;
                        for (int l = 0; l < blobPath.elementCount() - 1; l++) {
                            QPointF tp1 = blobPath.elementAt(l);
                            QPointF tp2 = blobPath.elementAt(l + 1);
                            double tSegmentLength = QLineF(tp1, tp2).length();

                            if (trailLength + tSegmentLength >= trailPosOnPath) {
                                double trailPointT = (trailPosOnPath - trailLength) / tSegmentLength;
                                QPointF trailPoint = tp1 * (1 - trailPointT) + tp2 * trailPointT;
                                marker.trailPoints.push_back(trailPoint);
                                break;
                            }
                            trailLength += tSegmentLength;
                        }
                    }
                }

                break;
            }
            currentLength += segmentLength;
        }

        // Rysuj wybrany efekt
        QColor markerColor = getMarkerColor(marker.markerType, marker.colorPhase);

        switch (marker.markerType) {
            case 0: // Impulsy energii
            {
                // Rysuj ślad impulsu
                if (!marker.trailPoints.empty()) {
                    for (size_t i = 0; i < marker.trailPoints.size(); i++) {
                        double fadeRatio = static_cast<double>(i) / marker.trailPoints.size();
                        QColor trailColor = markerColor;
                        trailColor.setAlphaF(1.0 - fadeRatio);

                        int pointSize = marker.size * (1.0 - fadeRatio * 0.7);
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(trailColor);
                        painter.drawEllipse(marker.trailPoints[i], pointSize, pointSize);
                    }
                }

                // Rysuj główny impuls
                painter.setPen(Qt::NoPen);
                painter.setBrush(markerColor);
                painter.drawEllipse(markerPos, marker.size * 1.5, marker.size * 1.5);

                // Dodaj jasny środek impulsu
                QColor centerColor = markerColor.lighter(150);
                painter.setBrush(centerColor);
                painter.drawEllipse(markerPos, marker.size * 0.7, marker.size * 0.7);
            }
            break;

            case 1: // Fale zakłóceń
            {
                if (marker.wavePhase > 0.0) {
                    double waveRadius = marker.size * 5 * marker.wavePhase;
                    double opacity = 1.0 - (marker.wavePhase / 5.0);

                    QColor waveColor = markerColor;
                    waveColor.setAlphaF(opacity * 0.7);

                    painter.setPen(QPen(waveColor, 0.8));
                    painter.setBrush(Qt::NoBrush);

                    // Rysuj koncentryczne fale
                    for (int i = 0; i < 3; i++) {
                        double ringRadius = waveRadius * (0.6 + 0.2 * i);
                        painter.drawEllipse(markerPos, ringRadius, ringRadius);
                    }

                    // Dodaj zniekształcenia fali
                    QPainterPath distortionPath;
                    int points = 12;
                    double noiseAmplitude = waveRadius * 0.15;

                    for (int i = 0; i <= points; i++) {
                        double angle = (2.0 * M_PI * i) / points;
                        double noise = QRandomGenerator::global()->bounded(noiseAmplitude);
                        double x = markerPos.x() + cos(angle) * (waveRadius + noise);
                        double y = markerPos.y() + sin(angle) * (waveRadius + noise);

                        if (i == 0)
                            distortionPath.moveTo(x, y);
                        else
                            distortionPath.lineTo(x, y);
                    }

                    distortionPath.closeSubpath();
                    waveColor.setAlphaF(opacity * 0.3);
                    painter.setPen(QPen(waveColor, 0.5, Qt::DotLine));
                    painter.drawPath(distortionPath);
                }

                // Zawsze rysuj punkt źródłowy fali
                painter.setPen(Qt::NoPen);
                painter.setBrush(markerColor);
                painter.drawEllipse(markerPos, marker.size * 0.8, marker.size * 0.8);
            }
            break;

            case 2: // Efekt quantum computing
            {
                // Generuj kilka "kopii" punktu dla efektu kwantowego
                QRandomGenerator *rng = QRandomGenerator::global();
                const int numQuantumCopies = 5;

                // Rysuj rozmyte punkty w różnych pozycjach
                for (int i = 0; i < numQuantumCopies; i++) {
                    // Wylicz pozycję względną dla kopii kwantowej
                    double quantumNoiseX = (rng->generateDouble() * 2.0 - 1.0) * marker.size * 2;
                    double quantumNoiseY = (rng->generateDouble() * 2.0 - 1.0) * marker.size * 2;

                    // Dodaj sinusoidalne przemieszczenie zależne od czasu
                    double timeOffset = marker.quantumOffset + currentTime * 0.001;
                    quantumNoiseX += sin(timeOffset * 3.0 + i) * marker.size * 0.8;
                    quantumNoiseY += cos(timeOffset * 2.5 + i) * marker.size * 0.8;

                    QPointF quantumPos = markerPos + QPointF(quantumNoiseX, quantumNoiseY);

                    // Ustaw kolor i przezroczystość
                    QColor quantumColor = markerColor;
                    quantumColor.setAlphaF(0.3 + 0.4 * (1.0 - static_cast<double>(i) / numQuantumCopies));

                    // Rysuj punkt kwantowy z efektem rozmycia
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(quantumColor);

                    double pointSize = marker.size * (0.8 + 0.4 * (1.0 - static_cast<double>(i) / numQuantumCopies));
                    painter.drawEllipse(quantumPos, pointSize, pointSize);

                    // Dodaj wewnętrzną poświatę
                    QColor glowColor = quantumColor.lighter(120);
                    glowColor.setAlphaF(quantumColor.alphaF() * 0.7);
                    painter.setBrush(glowColor);
                    painter.drawEllipse(quantumPos, pointSize * 0.5, pointSize * 0.5);
                }

                // Dodaj linie łączące kopie kwantowe (entanglement)
                painter.setPen(QPen(markerColor, 0.5, Qt::DotLine));
                for (int i = 0; i < numQuantumCopies - 1; i++) {
                    QPointF p1 = markerPos + QPointF(
                        sin(marker.quantumOffset + currentTime * 0.001 * 3.0 + i) * marker.size * 0.8,
                        cos(marker.quantumOffset + currentTime * 0.001 * 2.5 + i) * marker.size * 0.8
                    );

                    QPointF p2 = markerPos + QPointF(
                        sin(marker.quantumOffset + currentTime * 0.001 * 3.0 + i + 1) * marker.size * 0.8,
                        cos(marker.quantumOffset + currentTime * 0.001 * 2.5 + i + 1) * marker.size * 0.8
                    );

                    painter.drawLine(p1, p2);
                }
            }
            break;
        }
    }
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

    // Rysuj linie skanowania tylko w stanie IDLE
    if (animationState == BlobConfig::IDLE) {
        // Dodajmy subtelniejsze linie skanowania wewnątrz bloba
        painter.setClipPath(blobPath);
        painter.setPen(QPen(QColor(borderColor.lighter(150)), 0.5)); // Cieńsza linia (0.5 zamiast 1)

        // Zwiększamy odstęp między liniami z 4 na 12 (3x mniej linii)
        for (int y = 0; y < blobRadius * 2; y += 12) {
            int yPos = blobCenter.y() - blobRadius + y;
            QLineF line(blobCenter.x() - blobRadius, yPos, blobCenter.x() + blobRadius, yPos);
            painter.setOpacity(0.04); // Zmniejszam nieprzezroczystość z 0.1 na 0.04
            painter.drawLine(line);
        }
        painter.setOpacity(1.0);
        painter.setClipping(false);

        // Dodajmy cyfrowe artefakty/linie wewnątrz bloba
        painter.setClipPath(blobPath);

        QColor techColor = borderColor.lighter(150);
        techColor.setAlpha(30); // Zmniejszam z 40 na 30 dla mniejszej agresywności
        QPen techPen(techColor, 1, Qt::DotLine);
        painter.setPen(techPen);

        // Mniej linii wewnętrznych (z 3+4 na 2+2)
        QRandomGenerator *rng = QRandomGenerator::global();
        int numLines = 2 + rng->bounded(2);

        for (int i = 0; i < numLines; i++) {
            double angle = rng->bounded(2.0 * M_PI);
            double startDist = 0.2 * blobRadius;
            double endDist = 0.8 * blobRadius;

            QPointF start = blobCenter + QPointF(cos(angle) * startDist, sin(angle) * startDist);
            QPointF end = blobCenter + QPointF(cos(angle) * endDist, sin(angle) * endDist);

            painter.drawLine(start, end);
        }

        painter.setClipping(false);
    }
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
    } else if (renderState.isAbsorbing) {
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

    if (m_glitchIntensity > 0.01) {
        // Zachowaj oryginalną transformację
        painter.save();

        // Rysuj zakłócenia poziome
        QColor glitchColor = params.borderColor;
        glitchColor.setAlpha(120);
        painter.setPen(QPen(glitchColor, 2));

        int glitchCount = 2 + QRandomGenerator::global()->bounded(4);
        for (int i = 0; i < glitchCount; i++) {
            int yPos = QRandomGenerator::global()->bounded(height);
            int xOffset = QRandomGenerator::global()->bounded(width / 10);
            int glitchWidth = width / 4 + QRandomGenerator::global()->bounded(width / 2);

            painter.drawLine(xOffset, yPos, xOffset + glitchWidth, yPos);
        }

        // Niewielkie przesunięcie bloba dla efektu drgania
        if (m_glitchIntensity > 0.4 && QRandomGenerator::global()->bounded(100) < 30) {
            painter.translate(
                QRandomGenerator::global()->bounded(5) - 2,
                QRandomGenerator::global()->bounded(5) - 2
            );
        }

        painter.restore();
    }

    // Renderuj blob
    renderBlob(painter, controlPoints, blobCenter, params, width, height, renderState.animationState);

    if (renderState.animationState == BlobConfig::IDLE) {
        drawHUD(painter, blobCenter, params.blobRadius, params.borderColor, width, height);
    }

    // Przywróć poprzedni stan transformacji
    if (renderState.isBeingAbsorbed || renderState.isAbsorbing) {
        painter.restore();
    }
}

void BlobRenderer::drawHUD(QPainter &painter, const QPointF &blobCenter,
                           double blobRadius, const QColor &hudColor,
                           int width, int height) {
    QColor techColor = hudColor.lighter(120);
    techColor.setAlpha(180);
    painter.setPen(QPen(techColor, 1));
    painter.setFont(QFont("Consolas", 8));

    // Dodaj wskaźniki w rogach ekranu (styl AR)
    int cornerSize = 15;

    // Lewy górny
    painter.drawLine(10, 10, 10 + cornerSize, 10);
    painter.drawLine(10, 10, 10, 10 + cornerSize);
    painter.drawText(15, 25, "BLOB MONITOR");

    // Prawy górny
    painter.drawLine(width - 10 - cornerSize, 10, width - 10, 10);
    painter.drawLine(width - 10, 10, width - 10, 10 + cornerSize);
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    painter.drawText(width - 80, 25, timestamp);

    // Prawy dolny
    painter.drawLine(width - 10 - cornerSize, height - 10, width - 10, height - 10);
    painter.drawLine(width - 10, height - 10, width - 10, height - 10 - cornerSize);

    // Lewy dolny
    painter.drawLine(10, height - 10, 10 + cornerSize, height - 10);
    painter.drawLine(10, height - 10, 10, height - 10 - cornerSize);

    // Dodaj okrąg wokół bloba (cel)
    QPen targetPen(techColor, 1, Qt::DotLine);
    painter.setPen(targetPen);
    painter.drawEllipse(blobCenter, blobRadius + 20, blobRadius + 20);

    // Dodaj "identyfikator" bloba
    QString blobId = QString("BLOB-ID: %1").arg(QRandomGenerator::global()->bounded(1000, 9999));
    QFontMetrics fm(painter.font());
    int textWidth = fm.horizontalAdvance(blobId);
    painter.drawText(blobCenter.x() - textWidth / 2, blobCenter.y() + blobRadius + 30, blobId);

    // Dodaj informacje techniczne
    double amplitude = 1.5 + sin(QDateTime::currentMSecsSinceEpoch() * 0.001) * 0.5;
    QString amplitudeText = QString("AMP: %1").arg(amplitude, 0, 'f', 1);
    painter.drawText(15, height - 25, amplitudeText);

    QString sizeText = QString("R: %1").arg(int(blobRadius));
    painter.drawText(width - 70, height - 25, sizeText);
}

QColor BlobRenderer::getMarkerColor(int markerType, double colorPhase) {
    switch (markerType) {
    case 0: // Impulsy energii - intensywny elektryczny niebieski
        return QColor::fromHsvF(0.55 + 0.05 * sin(colorPhase * 2 * M_PI), 0.95, 0.99);

    case 1: // Fale zakłóceń - turkusowy do fioletowego
        return QColor::fromHsvF(0.48 + 0.15 * sin(colorPhase * 2 * M_PI), 0.9, 0.95);

    case 2: // Efekt kwantowy - głęboki niebieski z odcieniami fioletu
        return QColor::fromHsvF(0.65 + 0.08 * sin(colorPhase * 2 * M_PI), 0.85, 0.95);

    default:
        return QColor(0, 200, 255); // Domyślny niebieski neon
    }
}
