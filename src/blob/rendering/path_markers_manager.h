#ifndef PATH_MARKERS_MANAGER_H
#define PATH_MARKERS_MANAGER_H

#include <QPainter>
#include <vector>
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
    void initializeMarkers();

    // Aktualizuje stan markerów
    void updateMarkers(double deltaTime);

    // Rysuje markery na ścieżce
    void drawMarkers(QPainter &painter, const QPainterPath &blobPath, qint64 currentTime);

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
    void calculateTrailPoints(PathMarker &marker, const QPainterPath &blobPath, double pos, double pathLength);

    // Rysuje marker typu impuls energii
    void drawImpulseMarker(QPainter &painter, const PathMarker &marker, const QPointF &pos, const QColor &markerColor, qint64 currentTime);

    // Rysuje marker typu fala zakłóceń
    void drawWaveMarker(QPainter &painter, const PathMarker &marker, const QPointF &pos, const QColor &markerColor);

    // Rysuje marker typu quantum computing
    void drawQuantumMarker(QPainter &painter, const PathMarker &marker, const QPointF &pos, const QColor &markerColor, qint64 currentTime);

    // Funkcja zwracająca kolor markera w zależności od typu i fazy
    QColor getMarkerColor(int markerType, double colorPhase) const;
};

#endif // PATH_MARKERS_MANAGER_H