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
    int lastMarkerType = -1; // Zmienna śledząca ostatni wygenerowany typ

    for (int i = 0; i < numMarkers; i++) {
        PathMarker marker;
        marker.position = rng->bounded(1.0); // Pozycja 0.0-1.0 wzdłuż ścieżki

        // Logika zapobiegająca wylosowaniu tego samego typu dwa razy pod rząd
        int newMarkerType;
        if (lastMarkerType == -1) {
            // Pierwszy marker - losuj dowolny typ
            newMarkerType = rng->bounded(3);
        } else {
            // Kolejne markery - losuj typ różny od poprzedniego
            // Najpierw wybieramy losowo jeden z dwóch pozostałych typów
            int offset = 1 + rng->bounded(2);  // 1 lub 2
            newMarkerType = (lastMarkerType + offset) % 3;
        }

        marker.markerType = newMarkerType;
        lastMarkerType = newMarkerType; // Aktualizuj ostatni typ

        marker.size = 4 + rng->bounded(3); // Rozmiar bazowy znaczników (4-6)
        marker.direction = rng->bounded(2) * 2 - 1; // Losowo 1 lub -1
        marker.colorPhase = rng->bounded(1.0); // Losowa początkowa faza koloru
        marker.colorSpeed = 0.3 + rng->bounded(0.4); // Losowa prędkość zmiany koloru
        marker.tailLength = 0.02 + rng->bounded(0.03); // 2-5% długości ścieżki
        marker.wavePhase = 0.0; // Początkowa faza fali
        marker.quantumOffset = rng->bounded(10.0); // Losowe przesunięcie kwantowe

        // Ustaw prędkość w zależności od typu markera
        switch (marker.markerType) {
        case 0: // Impulsy energii - stała prędkość 0.05
            marker.speed = 0.1 + (rng->bounded(11) * 0.01);  // Poprawiona wartość
            break;
        case 1: // Pakiety (fale zakłóceń) - losowa 0.1-0.2 co 0.01
            marker.speed = 0.05;  // Poprawiona wartość
            break;
        case 2: // Quantum computing - losowa 0.14-0.2 co 0.02
            marker.speed = 0.14 + (rng->bounded(4) * 0.02); // 0.14, 0.16, 0.18, 0.2
            break;
        }

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
        marker.position += marker.direction * marker.speed * deltaTime;
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
                    marker.trailPoints.clear(); // Czyścimy poprzednie punkty śladu

                    int numTrailPoints = 15; // Liczba punktów w śladzie

                    for (int k = 0; k < numTrailPoints; k++) {
                        double trailT = static_cast<double>(k) / numTrailPoints;

                        // Kluczowa zmiana - uwzględniamy kierunek ruchu markera
                        double trailPosOnPath = pos - (marker.direction * trailT * marker.tailLength * pathLength);

                        // Zawijamy pozycję wokół ścieżki, jeśli wyjdzie poza zakres
                        if (trailPosOnPath < 0) trailPosOnPath += pathLength;
                        if (trailPosOnPath > pathLength) trailPosOnPath -= pathLength;

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

        switch (marker.markerType)
        {
        case 0: // Ulepszony efekt pakietu (impulsu energii)
            {
                // Rysuj ślad - to już działa dobrze
                if (!marker.trailPoints.empty()) {
                    for (size_t i = 0; i < marker.trailPoints.size(); i++) {
                        double fadeRatio = static_cast<double>(i) / marker.trailPoints.size();
                        QColor trailColor = markerColor;
                        trailColor.setAlphaF(0.8 - fadeRatio * 0.8);

                        int pointSize = marker.size * (1.0 - fadeRatio * 0.7);
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(trailColor);
                        painter.drawEllipse(marker.trailPoints[i], pointSize, pointSize);
                    }
                }

                // Nowa cyberpunkowa głowica pakietu
                QPointF pos = markerPos;
                const double headSize = marker.size * 1.8; // Podstawowy rozmiar głowicy

                // Warstwa 1: Podstawowy kształt - romb (diament)
                QPainterPath head;
                head.moveTo(pos.x(), pos.y() - headSize/2);        // góra
                head.lineTo(pos.x() + headSize/2, pos.y());        // prawo
                head.lineTo(pos.x(), pos.y() + headSize/2);        // dół
                head.lineTo(pos.x() - headSize/2, pos.y());        // lewo
                head.closeSubpath();

                // Główny gradient dla głowicy
                QRadialGradient headGradient(pos, headSize/2);
                QColor coreColor = markerColor.lighter(130);
                QColor edgeColor = markerColor;
                coreColor.setAlphaF(0.9);
                edgeColor.setAlphaF(0.7);
                headGradient.setColorAt(0, coreColor);
                headGradient.setColorAt(1, edgeColor);

                // Rysujemy główną głowicę
                painter.setPen(Qt::NoPen);
                painter.setBrush(headGradient);
                painter.drawPath(head);

                // Warstwa 2: Wewnętrzne linie technologiczne
                QColor techColor = markerColor.lighter(140);
                techColor.setAlphaF(0.85);
                painter.setPen(QPen(techColor, 0.8));

                // Wewnętrzne linie tworzące krzyż
                painter.drawLine(QPointF(pos.x() - headSize/3, pos.y()),
                                QPointF(pos.x() + headSize/3, pos.y()));
                painter.drawLine(QPointF(pos.x(), pos.y() - headSize/3),
                                QPointF(pos.x(), pos.y() + headSize/3));

                // Warstwa 3: Centralny punkt energii
                QColor centerColor = QColor::fromHsvF(
                    markerColor.hsvHueF(),
                    markerColor.hsvSaturationF() * 0.6,
                    qMin(1.0, markerColor.valueF() * 1.4)
                );
                centerColor.setAlphaF(0.95);

                painter.setPen(Qt::NoPen);
                painter.setBrush(centerColor);
                double innerSize = marker.size * 0.5;

                // Małe migotanie rozmiaru centralnego punktu
                double time = QDateTime::currentMSecsSinceEpoch() * 0.002;
                innerSize *= (0.9 + 0.2 * sin(time + marker.position * 10));

                // Rysowanie centralnego sześciokąta zamiast koła
                QPainterPath centerHex;
                for (int i = 0; i < 6; i++) {
                    double angle = 2.0 * M_PI * i / 6;
                    double x = pos.x() + innerSize * cos(angle);
                    double y = pos.y() + innerSize * sin(angle);

                    if (i == 0)
                        centerHex.moveTo(x, y);
                    else
                        centerHex.lineTo(x, y);
                }
                centerHex.closeSubpath();
                painter.drawPath(centerHex);

                // Warstwa 4: Efekt migotania na brzegach (glitch)
                if (QRandomGenerator::global()->bounded(100) < 30) {
                    double glitchAngle = QRandomGenerator::global()->bounded(2.0 * M_PI);
                    double glitchDist = headSize/2 * 0.9;
                    QPointF glitchPos(
                        pos.x() + cos(glitchAngle) * glitchDist,
                        pos.y() + sin(glitchAngle) * glitchDist
                    );

                    QColor glitchColor = markerColor.lighter(150);
                    glitchColor.setAlphaF(0.7);
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(glitchColor);
                    painter.drawEllipse(glitchPos, marker.size * 0.3, marker.size * 0.3);
                }
            }
            break;

        case 1: // Fale zakłóceń - zmniejszone
            {
                if (marker.wavePhase > 0.0) {
                    // Zmniejszony promień fali o 60% (z 5 na 2)
                    double waveRadius = marker.size * 1.5 * marker.wavePhase;
                    double opacity = 1.0 - (marker.wavePhase / 5.0);

                    QColor waveColor = markerColor;
                    waveColor.setAlphaF(opacity * 0.7);

                    painter.setPen(QPen(waveColor, 0.6)); // Cieńsza linia
                    painter.setBrush(Qt::NoBrush);

                    // Rysuj delikatniejsze fale
                    for (int i = 0; i < 2; i++) { // Zmniejszone z 3 do 2 pierścieni
                        double ringRadius = waveRadius * (0.7 + 0.3 * i);
                        painter.drawEllipse(markerPos, ringRadius, ringRadius);
                    }

                    // Mniej zaburzony kształt fali
                    QPainterPath distortionPath;
                    int points = 12;
                    // Zmniejszenie amplitudy zniekształcenia o 50%
                    double noiseAmplitude = waveRadius * 0.07;

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
                    waveColor.setAlphaF(opacity * 0.25); // Mniejsza nieprzezroczystość
                    painter.setPen(QPen(waveColor, 0.3, Qt::DotLine)); // Cieńsza linia
                    painter.drawPath(distortionPath);
                }

                // Punkt źródłowy fali - niezmienny
                painter.setPen(Qt::NoPen);
                painter.setBrush(markerColor);
                painter.drawEllipse(markerPos, marker.size * 0.8, marker.size * 0.8);
            }
            break;

        case 2: // Efekt quantum computing - poprawiony
            {
                // Płynniejszy efekt kwantowy z mniej przypadkowymi pozycjami
                const int numQuantumCopies = 5;
                QPainterPath quantumPath;
                QVector<QPointF> quantumPoints;

                // Bardziej spójne punkty kwantowe
                for (int i = 0; i < numQuantumCopies; i++) {
                    // Bardziej płynne przejście między punktami zamiast całkowicie losowych pozycji
                    double theta = (2.0 * M_PI * i) / numQuantumCopies;
                    double phase = currentTime * 0.0005 + marker.quantumOffset;

                    // Płynna animacja po eliptycznej orbicie z lekką zmiennością
                    double orbitA = marker.size * (1.2 + 0.6 * sin(phase * 1.5 + i * 0.7));
                    double orbitB = marker.size * (1.0 + 0.4 * sin(phase * 2.0 + i * 0.5));

                    // Płynne przejście pozycji z dodatkowym subtelnym efektem pulsacji
                    double x = sin(theta + phase) * orbitA;
                    double y = cos(theta + phase * 1.3) * orbitB;

                    QPointF quantumPos = markerPos + QPointF(x, y);
                    quantumPoints.append(quantumPos);

                    // Ustaw kolor z efektem pulsacji
                    QColor quantumColor = markerColor;
                    double pulseIntensity = 0.4 + 0.6 * (0.5 + 0.5 * sin(phase * 3.0 + i));
                    quantumColor.setAlphaF(0.4 * pulseIntensity);

                    // Rysuj punkt kwantowy z efektem rozmycia
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(quantumColor);

                    // Płynnie zmieniający się rozmiar
                    double pointSize = marker.size * (0.6 + 0.3 * pulseIntensity);
                    painter.drawEllipse(quantumPos, pointSize, pointSize);

                    // Wewnętrzna poświata - zwiększona jasność, zmniejszona przezroczystość
                    QColor glowColor = quantumColor.lighter(130);
                    glowColor.setAlphaF(quantumColor.alphaF() * 0.8);
                    painter.setBrush(glowColor);
                    painter.drawEllipse(quantumPos, pointSize * 0.5, pointSize * 0.5);

                    // Zbieramy punkty do ścieżki
                    if (i == 0)
                        quantumPath.moveTo(quantumPos);
                    else
                        quantumPath.lineTo(quantumPos);
                }

                // Zamknij ścieżkę
                if (!quantumPoints.isEmpty())
                    quantumPath.lineTo(quantumPoints.first());

                // Rysuj delikatną ścieżkę łączącą punkty (efekt splątania kwantowego)
                QColor pathColor = markerColor;
                pathColor.setAlphaF(0.2);
                painter.setPen(QPen(pathColor, 0.5, Qt::DotLine));
                painter.setBrush(Qt::NoBrush);
                painter.drawPath(quantumPath);

                // Dodaj centralny punkt
                painter.setBrush(markerColor.lighter(110));
                painter.drawEllipse(markerPos, marker.size * 0.4, marker.size * 0.4);
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
    case 0: // Impulsy energii - Neonowa zieleń
        return QColor::fromHsvF(0.33 + 0.05 * sin(colorPhase * 2 * M_PI), 1.0, 0.95);

    case 1: // Fale zakłóceń - Złoto-żółty
        return QColor::fromHsvF(0.13 + 0.07 * sin(colorPhase * 2 * M_PI), 0.9, 0.95);

    case 2: // Efekt kwantowy - Neo-purpura
        return QColor::fromHsvF(0.78 + 0.1 * sin(colorPhase * 2 * M_PI), 0.85, 1.0);

    default:
        return QColor(0, 200, 255); // Domyślny niebieski neon
    }
}
