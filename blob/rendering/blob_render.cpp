#include "blob_render.h"

#include <QDateTime>
#include <QRadialGradient>
#include <QRandomGenerator>

void BlobRenderer::renderBlob(QPainter &painter,
                              const std::vector<QPointF> &controlPoints,
                              const QPointF &blobCenter,
                              const BlobConfig::BlobParameters &params,
                              int width, int height) {
    painter.setRenderHint(QPainter::Antialiasing, true);


    QPainterPath blobPath = BlobPath::createBlobPath(controlPoints, controlPoints.size());

    drawGlowEffect(painter, blobPath, params.borderColor, params.glowRadius);

    drawBorder(painter, blobPath, params.borderColor, params.borderWidth);

    drawFilling(painter, blobPath, blobCenter, params.blobRadius, params.borderColor);
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
        bufferPainter.setPen(QPen(gridColor, 1, Qt::SolidLine));

        // Główne linie siatki
        for (int y = 0; y < height; y += gridSpacing) {
            bufferPainter.drawLine(0, y, width, y);
        }

        for (int x = 0; x < width; x += gridSpacing) {
            bufferPainter.drawLine(x, 0, x, height);
        }

        // Dodajemy podsiatkę o mniejszej intensywności
        QColor subgridColor = gridColor;
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
            marker.markerType = rng->bounded(3); // 0-heksagon, 1-trójkąt, 2-okrąg
            marker.size = 4; // Rozmiar bazowy znaczników
            marker.direction = rng->bounded(2) * 2 - 1; // Losowo 1 lub -1
            marker.colorPhase = rng->bounded(1.0); // Losowa początkowa faza koloru
            marker.colorSpeed = 0.3 + rng->bounded(0.4); // Losowa prędkość zmiany koloru
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
    }

    // Rysuj markery w ich aktualnych pozycjach
    for (const auto &marker: m_pathMarkers) {
        double pos = marker.position * pathLength;
        double currentLength = 0;

        // Znajdź punkt na ścieżce
        for (int j = 0; j < blobPath.elementCount() - 1; j++) {
            QPointF p1 = blobPath.elementAt(j);
            QPointF p2 = blobPath.elementAt(j + 1);
            double segmentLength = QLineF(p1, p2).length();

            if (currentLength + segmentLength >= pos) {
                double t = (pos - currentLength) / segmentLength;
                QPointF markerPos = p1 * (1 - t) + p2 * t;

                // Rysuj znacznik w stylu cyberpunk
                QColor markerColor = getMarkerColor(marker.markerType, marker.colorPhase);
                painter.setPen(QPen(markerColor, 2));

                QRectF markerRect(markerPos.x() - marker.size / 2, markerPos.y() - marker.size / 2,
                                  marker.size, marker.size);

                // W metodzie drawBorder, zmień fragment switch-case rysujący znaczniki na:

                // Rysuj wybrany styl znacznika - symbole korporacyjne
                switch (marker.markerType) {
                    case 0: // Heksagonalne logo
                    {
                        int size = marker.size * 2;

                        // Rysuj sześciokąt
                        QPainterPath hexPath;
                        for (int i = 0; i < 6; i++) {
                            double angle = i * M_PI / 3.0;
                            if (i == 0) {
                                hexPath.moveTo(markerPos.x() + size / 2 * cos(angle),
                                               markerPos.y() + size / 2 * sin(angle));
                            } else {
                                hexPath.lineTo(markerPos.x() + size / 2 * cos(angle),
                                               markerPos.y() + size / 2 * sin(angle));
                            }
                        }
                        hexPath.closeSubpath();
                        painter.drawPath(hexPath);

                        // Wewnętrzny symbol korporacji (styl "X")
                        double innerSize = size * 0.4;
                        painter.drawLine(markerPos.x() - innerSize / 2, markerPos.y() - innerSize / 2,
                                         markerPos.x() + innerSize / 2, markerPos.y() + innerSize / 2);
                        painter.drawLine(markerPos.x() - innerSize / 2, markerPos.y() + innerSize / 2,
                                         markerPos.x() + innerSize / 2, markerPos.y() - innerSize / 2);
                    }
                    break;

                    case 1: // Trójkątna odznaka
                    {
                        int size = marker.size * 2;

                        // Rysuj trójkąt
                        QPainterPath trianglePath;
                        trianglePath.moveTo(markerPos.x(), markerPos.y() - size / 2);
                        trianglePath.lineTo(markerPos.x() - size / 2, markerPos.y() + size / 2);
                        trianglePath.lineTo(markerPos.x() + size / 2, markerPos.y() + size / 2);
                        trianglePath.closeSubpath();
                        painter.drawPath(trianglePath);

                        // Linia przecinająca
                        painter.drawLine(markerPos.x() - size / 2 + size / 4, markerPos.y(),
                                         markerPos.x() + size / 2 - size / 4, markerPos.y());

                        // Mały punkt w środku
                        painter.setBrush(markerColor);
                        painter.drawEllipse(markerPos, size / 10, size / 10);
                        painter.setBrush(Qt::NoBrush);
                    }
                    break;

                    case 2: // Okręg z kodem kreskowym
                    {
                        int size = marker.size * 2;
                        QRectF circleRect(markerPos.x() - size / 2, markerPos.y() - size / 2, size, size);

                        // Rysuj okrąg
                        painter.drawEllipse(circleRect);

                        // Rysuj linie kodu kreskowego wewnątrz
                        painter.save();
                        painter.setClipPath(QPainterPath());
                        painter.setClipRegion(QRegion(circleRect.toRect(), QRegion::Ellipse));

                        double barWidth = size / 12.0;
                        double startX = markerPos.x() - size / 2 + barWidth;

                        // Rysuj alternatywnie różnej grubości linie (kod kreskowy)
                        for (int i = 0; i < 5; i++) {
                            double x = startX + i * barWidth * 2;
                            double height = size * (0.4 + (i % 3) * 0.15);
                            double y1 = markerPos.y() - height / 2;
                            double y2 = markerPos.y() + height / 2;

                            painter.drawLine(QPointF(x, y1), QPointF(x, y2));
                        }

                        painter.restore();
                    }
                    break;
                }

                break;
            }

            currentLength += segmentLength;
        }
    }
}


void BlobRenderer::drawFilling(QPainter &painter,
                               const QPainterPath &blobPath,
                               const QPointF &blobCenter,
                               double blobRadius,
                               const QColor &borderColor) {
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
    renderBlob(painter, controlPoints, blobCenter, params, width, height);

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
        case 0: // Heksagon - zielono-niebieskawa gama
            return QColor::fromHsvF(0.45 + 0.1 * sin(colorPhase * 2 * M_PI), 0.9, 0.95);

        case 1: // Trójkąt - żółto-pomarańczowa gama
            return QColor::fromHsvF(0.11 + 0.05 * sin(colorPhase * 2 * M_PI), 0.9, 0.95);

        case 2: // Okrąg - czerwono-różowa gama
            return QColor::fromHsvF(0.93 + 0.07 * sin(colorPhase * 2 * M_PI), 0.9, 0.95);

        default:
            return QColor(255, 255, 255); // Biały jako fallback
    }
}
