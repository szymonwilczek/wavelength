#ifndef PATH_MARKERS_MANAGER_H
#define PATH_MARKERS_MANAGER_H

#include <QColor>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPainter>
#include <QPainterPath>
#include <vector>
#include <cmath>
#include <qmath.h>

class PathMarkersManager {
public:
    struct PathMarker {
        double position;      // Pozycja na ścieżce (0.0-1.0)
        int markerType;       // 0-impulsy energii, 1-fale zakłóceń, 2-efekt kwantowy
        int size;             // Rozmiar znacznika
        int direction;        // Kierunek ruchu (1 lub -1)
        double colorPhase;    // Faza koloru (0.0-1.0)
        double colorSpeed;    // Prędkość zmiany koloru
        double tailLength;    // Długość śladu impulsu (tylko dla typu 0)
        double wavePhase;     // Faza fali zakłóceń (tylko dla typu 1)
        double quantumOffset; // Przesunięcie kwantowe (tylko dla typu 2)
        double speed;         // Indywidualna prędkość markera
        
        // Pola dla cyklu kwantowego
        int quantumState;     // 0-pojedynczy, 1-rozwijanie, 2-rozczłonkowany, 3-zwijanie
        double quantumStateTime;    // Czas spędzony w bieżącym stanie
        double quantumStateDuration; // Planowana długość bieżącego stanu
        
        std::vector<QPointF> trailPoints; // Punkty śladu dla impulsu energii
    };
    
    PathMarkersManager() : m_lastUpdateTime(0) {}
    
    // Inicjalizuje lub resetuje zestaw markerów
    void initializeMarkers() {
        m_markers.clear();
        m_lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
        
        QRandomGenerator *rng = QRandomGenerator::global();
        int numMarkers = rng->bounded(4, 7); // Liczba markerów
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
                marker.speed = 0.1 + (rng->bounded(11) * 0.01);  // Losowa prędkość
                break;
            case 1: // Pakiety (fale zakłóceń) - losowa 0.1-0.2 co 0.01
                marker.speed = 0.05;  // Stała prędkość
                break;
            case 2: // Quantum computing - losowa 0.14-0.2 co 0.02
                marker.speed = 0.14 + (rng->bounded(4) * 0.02); // 0.14, 0.16, 0.18, 0.2
                break;
            }
            
            if (marker.markerType == 2) { // Quantum computing
                marker.quantumState = 0;  // Startuj jako pojedynczy punkt
                marker.quantumStateTime = 0.0;
                // Losowy czas trwania pierwszego stanu (2-4 sekundy)
                marker.quantumStateDuration = 2.0 + rng->bounded(2.0);
            } else {
                marker.quantumState = 0;
                marker.quantumStateTime = 0.0;
                marker.quantumStateDuration = 0.0;
            }
            
            m_markers.push_back(marker);
        }
    }
    
    // Aktualizuje stan markerów
    void updateMarkers(double deltaTime) {
        QRandomGenerator *rng = QRandomGenerator::global();
        
        // Przesuń markery wzdłuż ścieżki
        for (auto &marker: m_markers) {
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
            
            // Aktualizacja stanu kwantowego
            if (marker.markerType == 2) {
                marker.quantumStateTime += deltaTime;
                
                // Sprawdź, czy czas w bieżącym stanie minął
                if (marker.quantumStateTime >= marker.quantumStateDuration) {
                    // Przejdź do następnego stanu
                    marker.quantumState = (marker.quantumState + 1) % 4;
                    marker.quantumStateTime = 0.0;
                    
                    // Ustaw losowy czas trwania nowego stanu
                    switch (marker.quantumState) {
                    case 0: // Pojedynczy punkt
                        marker.quantumStateDuration = 2.0 + rng->bounded(2.0); // 2-4 sekundy
                        break;
                    case 1: // Rozwijanie
                        marker.quantumStateDuration = 0.8 + rng->bounded(0.6); // 0.8-1.4 sekundy
                        break;
                    case 2: // Rozczłonkowany
                        marker.quantumStateDuration = 1.5 + rng->bounded(1.5); // 1.5-3 sekundy
                        break;
                    case 3: // Zwijanie
                        marker.quantumStateDuration = 0.8 + rng->bounded(0.6); // 0.8-1.4 sekundy
                        break;
                    }
                }
            }
            
            // Przygotowanie do obliczenia śladu
            if (marker.markerType == 0) {
                marker.trailPoints.clear();
            }
        }
    }
    
    // Rysuje markery na ścieżce
    void drawMarkers(QPainter &painter, const QPainterPath &blobPath, qint64 currentTime) {
        if (m_markers.empty()) {
            initializeMarkers();
        }
        
        // Oblicz deltaTime
        double deltaTime = (currentTime - m_lastUpdateTime) / 1000.0; // Czas w sekundach
        m_lastUpdateTime = currentTime;
        
        // Aktualizacja wszystkich markerów
        updateMarkers(deltaTime);
        
        // Oblicz długość ścieżki
        double pathLength = 0;
        for (int i = 0; i < blobPath.elementCount() - 1; i++) {
            QPointF p1 = blobPath.elementAt(i);
            QPointF p2 = blobPath.elementAt(i + 1);
            pathLength += QLineF(p1, p2).length();
        }
        
        // Rysuj markery w ich aktualnych pozycjach
        for (auto &marker: m_markers) {
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
                        calculateTrailPoints(marker, blobPath, pos, pathLength);
                    }
                    
                    break;
                }
                currentLength += segmentLength;
            }
            
            // Rysuj wybrany efekt
            QColor markerColor = getMarkerColor(marker.markerType, marker.colorPhase);
            
            switch (marker.markerType) {
                case 0: 
                    drawImpulseMarker(painter, marker, markerPos, markerColor, currentTime);
                    break;
                case 1: 
                    drawWaveMarker(painter, marker, markerPos, markerColor);
                    break;
                case 2: 
                    drawQuantumMarker(painter, marker, markerPos, markerColor, currentTime);
                    break;
            }
        }
    }
    
    // Getter dla liczby markerów
    size_t getMarkerCount() const {
        return m_markers.size();
    }
    
    // Getter dla konkretnego markera
    const PathMarker& getMarker(size_t index) const {
        return m_markers.at(index);
    }
    
private:
    std::vector<PathMarker> m_markers;
    qint64 m_lastUpdateTime;
    
    // Oblicza punkty śladu dla markera typu impuls
    void calculateTrailPoints(PathMarker &marker, const QPainterPath &blobPath, double pos, double pathLength) {
        marker.trailPoints.clear();
        int numTrailPoints = 15; // Liczba punktów w śladzie
        
        for (int k = 0; k < numTrailPoints; k++) {
            double trailT = static_cast<double>(k) / numTrailPoints;
            
            // Uwzględniamy kierunek ruchu markera
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
    
    // Rysuje marker typu impuls energii
    void drawImpulseMarker(QPainter &painter, const PathMarker &marker, const QPointF &pos, const QColor &markerColor, qint64 currentTime) {
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
        double time = currentTime * 0.002;
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
    
    // Rysuje marker typu fala zakłóceń
    void drawWaveMarker(QPainter &painter, const PathMarker &marker, const QPointF &pos, const QColor &markerColor) {
        if (marker.wavePhase > 0.0) {
            // Zmniejszony promień fali
            double waveRadius = marker.size * 1.5 * marker.wavePhase;
            double opacity = 1.0 - (marker.wavePhase / 5.0);
            
            QColor waveColor = markerColor;
            waveColor.setAlphaF(opacity * 0.7);
            
            painter.setPen(QPen(waveColor, 0.6)); // Cieńsza linia
            painter.setBrush(Qt::NoBrush);
            
            // Rysuj delikatniejsze fale
            for (int i = 0; i < 2; i++) {
                double ringRadius = waveRadius * (0.7 + 0.3 * i);
                painter.drawEllipse(pos, ringRadius, ringRadius);
            }
            
            // Mniej zaburzony kształt fali
            QPainterPath distortionPath;
            int points = 12;
            // Zmniejszenie amplitudy zniekształcenia
            double noiseAmplitude = waveRadius * 0.07;
            
            for (int i = 0; i <= points; i++) {
                double angle = (2.0 * M_PI * i) / points;
                double noise = QRandomGenerator::global()->bounded(noiseAmplitude);
                double x = pos.x() + cos(angle) * (waveRadius + noise);
                double y = pos.y() + sin(angle) * (waveRadius + noise);
                
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
        painter.drawEllipse(pos, marker.size * 0.8, marker.size * 0.8);
    }
    
    // Rysuje marker typu quantum computing
    void drawQuantumMarker(QPainter &painter, const PathMarker &marker, const QPointF &pos, const QColor &markerColor, qint64 currentTime) {
        // Każdy stan ma inny sposób renderowania
        double phase = currentTime * 0.0005 + marker.quantumOffset;
        
        // Oblicz współczynnik przejścia dla stanów rozwijaniai zwijania
        double transitionFactor = 0.0;
        if (marker.quantumState == 1) { // Rozwijanie
            transitionFactor = marker.quantumStateTime / marker.quantumStateDuration;
        } else if (marker.quantumState == 3) { // Zwijanie
            transitionFactor = 1.0 - marker.quantumStateTime / marker.quantumStateDuration;
        }
        
        // Podstawowy marker - widoczny zawsze
        QColor baseColor = markerColor;
        if (marker.quantumState == 0) {
            // W stanie pojedynczym używamy pełnej nieprzezroczystości
            baseColor.setAlphaF(0.9);
        } else {
            // W innych stanach ustawiamy odpowiednią nieprzezroczystość
            baseColor.setAlphaF(0.7);
        }
        
        // Rysuj podstawowy punkt
        painter.setPen(Qt::NoPen);
        painter.setBrush(baseColor);
        painter.drawEllipse(pos, marker.size * 1.0, marker.size * 1.0);
        
        // Dodaj wewnętrzny punkt (zawsze widoczny)
        QColor innerColor = markerColor.lighter(130);
        innerColor.setAlphaF(baseColor.alphaF() * 0.9);
        painter.setBrush(innerColor);
        painter.drawEllipse(pos, marker.size * 0.5, marker.size * 0.5);
        
        // Jeśli nie jest w stanie pojedynczym (0), rysuj efekty kwantowe
        if (marker.quantumState > 0) {
            const int numQuantumCopies = 5;
            QPainterPath quantumPath;
            QVector<QPointF> quantumPoints;
            
            // Rysuj kwantowe kopie punktu
            for (int i = 0; i < numQuantumCopies; i++) {
                // Kąt dla każdej kopii
                double theta = (2.0 * M_PI * i) / numQuantumCopies;
                
                // Oblicz promień orbity w zależności od stanu
                double orbitFactor = 0.0;
                
                // W stanie pełnego rozczłonkowania (2) lub podczas rozwijania/zwijania
                if (marker.quantumState == 2) {
                    orbitFactor = 1.0; // Pełny promień
                } else if (marker.quantumState == 1 || marker.quantumState == 3) {
                    orbitFactor = transitionFactor; // Stopniowo zwiększany/zmniejszany
                }
                
                // Oblicz orbity dla kopii
                double orbitA = marker.size * (1.2 + 0.6 * sin(phase * 1.5 + i * 0.7)) * orbitFactor;
                double orbitB = marker.size * (1.0 + 0.4 * sin(phase * 2.0 + i * 0.5)) * orbitFactor;
                
                // Pozycja kopii
                double x = sin(theta + phase) * orbitA;
                double y = cos(theta + phase * 1.3) * orbitB;
                
                QPointF quantumPos = pos + QPointF(x, y);
                quantumPoints.append(quantumPos);
                
                // Oblicz intensywność i nieprzezroczystość
                double pulseIntensity = 0.4 + 0.6 * (0.5 + 0.5 * sin(phase * 3.0 + i));
                double alpha = 0.4 * pulseIntensity * orbitFactor;  // Nieprzezroczystość zależna od stanu
                
                // Kolor dla kopii
                QColor quantumColor = markerColor;
                quantumColor.setAlphaF(alpha);
                
                // Rysuj kopię kwantową
                painter.setPen(Qt::NoPen);
                painter.setBrush(quantumColor);
                
                // Rozmiar kopii
                double pointSize = marker.size * (0.6 + 0.3 * pulseIntensity) * (0.6 + 0.4 * orbitFactor);
                painter.drawEllipse(quantumPos, pointSize, pointSize);
                
                // Wewnętrzny punkt dla kopii
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
            
            // Rysuj linie splątania kwantowego z odpowiednią przezroczystością
            if (!quantumPoints.isEmpty()) {
                QColor pathColor = markerColor;
                pathColor.setAlphaF(0.2 * (marker.quantumState == 2 ? 1.0 : transitionFactor));
                painter.setPen(QPen(pathColor, 0.5, Qt::DotLine));
                painter.setBrush(Qt::NoBrush);
                painter.drawPath(quantumPath);
            }
        }
    }
    
    // Funkcja zwracająca kolor markera w zależności od typu i fazy
    QColor getMarkerColor(int markerType, double colorPhase) const {
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
};

#endif // PATH_MARKERS_MANAGER_H