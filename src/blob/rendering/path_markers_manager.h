#ifndef PATH_MARKERS_MANAGER_H
#define PATH_MARKERS_MANAGER_H

#include <QPainter>
#include <vector>
#include <qmath.h> // For M_PI

/**
 * @brief Manages the creation, update, and rendering of animated markers along a QPainterPath.
 *
 * This class creates several types of markers (energy impulses, disturbance waves, quantum effects)
 * that move along a given path (typically the blob's outline). Each marker type has unique
 * visual characteristics and animation behavior. The manager handles marker initialization,
 * position updates based on time, and drawing logic using QPainter.
 */
class PathMarkersManager {
public:
    /**
     * @brief Structure representing a single animated marker moving along the path.
     */
    struct PathMarker {
        double position;            ///< Current position along the path, normalized (0.0 to 1.0).
        int markerType;             ///< Type of marker: 0=Impulse, 1=Wave, 2=Quantum.
        int size;                   ///< Base size of the marker in pixels.
        int direction;              ///< Direction of movement along the path (1 or -1).
        double colorPhase;          ///< Current phase for color cycling (0.0 to 1.0).
        double colorSpeed;          ///< Speed at which the color phase changes.
        double tailLength;          ///< Length of the trail for impulse markers (type 0), normalized to path length.
        double wavePhase;           ///< Current phase/radius of the disturbance wave (type 1).
        double quantumOffset;       ///< Random offset for quantum effect timing (type 2).
        double speed;               ///< Individual movement speed of the marker along the path.

        // Fields for quantum cycle (type 2)
        int quantumState;           ///< Current state: 0=Single, 1=Expanding, 2=Fragmented, 3=Collapsing.
        double quantumStateTime;    ///< Time elapsed in the current quantum state.
        double quantumStateDuration;///< Total duration planned for the current quantum state.

        std::vector<QPointF> trailPoints; ///< Pre-calculated points for rendering the trail of impulse markers (type 0).
    };

    /**
     * @brief Constructs a PathMarkersManager. Initializes last update time to 0.
     */
    PathMarkersManager() : m_lastUpdateTime(0) {}

    /**
     * @brief Initializes or resets the set of markers.
     * Clears existing markers and creates a new random set with varying types, speeds, and appearances.
     * Ensures that the same marker type is not generated twice consecutively.
     */
    void initializeMarkers();

    /**
     * @brief Updates the state of all markers based on the elapsed time.
     * Calculates new positions, color phases, wave phases, and quantum states.
     * @param deltaTime Time elapsed since the last update in seconds.
     */
    void updateMarkers(double deltaTime);

    /**
     * @brief Draws all managed markers onto the provided QPainter.
     * Calculates the time delta since the last draw call, updates marker states,
     * determines marker positions on the given path, and calls the appropriate
     * drawing function for each marker type. Initializes markers if the list is empty.
     * @param painter The QPainter to use for drawing.
     * @param blobPath The QPainterPath along which the markers should be drawn.
     * @param currentTime The current timestamp in milliseconds (e.g., from QDateTime::currentMSecsSinceEpoch()).
     */
    void drawMarkers(QPainter &painter, const QPainterPath &blobPath, qint64 currentTime);

    /**
     * @brief Gets the current number of active markers.
     * @return The number of markers in the m_markers vector.
     */
    size_t getMarkerCount() const {
        return m_markers.size();
    }

    /**
     * @brief Gets a constant reference to a specific marker by its index.
     * Performs bounds checking via std::vector::at().
     * @param index The index of the desired marker.
     * @return A constant reference to the PathMarker struct.
     * @throws std::out_of_range if the index is invalid.
     */
    const PathMarker& getMarker(const size_t index) const {
        // Note: Using .at() for bounds checking, will throw std::out_of_range if index is invalid.
        return m_markers.at(index);
    }

private:
    /** @brief Vector storing all the active PathMarker objects. */
    std::vector<PathMarker> m_markers;
    /** @brief Timestamp (milliseconds since epoch) of the last call to drawMarkers, used for calculating deltaTime. */
    qint64 m_lastUpdateTime;

    /**
     * @brief Calculates the points needed to draw the trailing effect for an impulse marker.
     * Populates the marker's trailPoints vector.
     * @param marker Reference to the PathMarker (type 0) whose trail points are being calculated (modified).
     * @param blobPath The path along which the marker moves.
     * @param pos The current absolute position (distance along the path) of the marker head.
     * @param pathLength The total length of the blobPath.
     */
    static void calculateTrailPoints(PathMarker &marker, const QPainterPath &blobPath, double pos, double pathLength);

    /**
     * @brief Draws a marker of type 0 (Impulse).
     * Renders a cyberpunk-style head element and a fading trail using pre-calculated points.
     * @param painter The QPainter to use for drawing.
     * @param marker The PathMarker data.
     * @param pos The calculated position (QPointF) on the painter where the marker head should be drawn.
     * @param markerColor The base color for the marker.
     * @param currentTime The current timestamp for potential time-based effects (e.g., flickering).
     */
    static void drawImpulseMarker(QPainter &painter, const PathMarker &marker, const QPointF &pos, const QColor &markerColor, qint64 currentTime);

    /**
     * @brief Draws a marker of type 1 (Wave).
     * Renders expanding concentric rings and a subtle distortion effect originating from the marker position.
     * @param painter The QPainter to use for drawing.
     * @param marker The PathMarker data (uses wavePhase).
     * @param pos The calculated position (QPointF) on the painter, representing the wave's origin.
     * @param markerColor The base color for the marker.
     */
    static void drawWaveMarker(QPainter &painter, const PathMarker &marker, const QPointF &pos, const QColor &markerColor);

    /**
     * @brief Draws a marker of type 2 (Quantum).
     * Renders a central point and several orbiting "quantum copies" whose appearance and behavior
     * change based on the marker's quantumState and quantumStateTime. Includes connecting lines ("entanglement").
     * @param painter The QPainter to use for drawing.
     * @param marker The PathMarker data (uses quantumState, quantumStateTime, etc.).
     * @param pos The calculated position (QPointF) on the painter for the central point.
     * @param markerColor The base color for the marker.
     * @param currentTime The current timestamp for time-based animation effects.
     */
    static void drawQuantumMarker(QPainter &painter, const PathMarker &marker, const QPointF &pos, const QColor &markerColor, qint64 currentTime);

    /**
     * @brief Determines the appropriate color for a marker based on its type and current color phase.
     * Uses HSV color manipulation for smooth transitions.
     * @param markerType The type of the marker (0, 1, or 2).
     * @param colorPhase The current color phase (0.0 to 1.0).
     * @return The calculated QColor for the marker.
     */
    static QColor getMarkerColor(int markerType, double colorPhase);
};

#endif // PATH_MARKERS_MANAGER_H