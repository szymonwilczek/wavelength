#ifndef PATH_MARKERS_MANAGER_H
#define PATH_MARKERS_MANAGER_H

#include <qglobal.h>
#include <vector>

class QColor;
class QPainterPath;
class QPainter;
class QPointF;

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
        double position; ///< Current position along the path, normalized (0.0 to 1.0).
        int marker_type; ///< Type of marker: 0=Impulse, 1=Wave, 2=Quantum.
        int size; ///< Base size of the marker in pixels.
        int direction; ///< Direction of movement along the path (1 or -1).
        double color_phase; ///< Current phase for color cycling (0.0 to 1.0).
        double color_speed; ///< Speed at which the color phase changes.
        double tail_length; ///< Length of the trail for impulse markers (type 0), normalized to path length.
        double wave_phase; ///< Current phase/radius of the disturbance wave (type 1).
        double quantum_offset; ///< Random offset for quantum effect timing (type 2).
        double speed; ///< Individual movement speed of the marker along the path.

        // Fields for quantum cycle (type 2)
        int quantum_state; ///< Current state: 0=Single, 1=Expanding, 2=Fragmented, 3=Collapsing.
        double quantum_state_time; ///< Time elapsed in the current quantum state.
        double quantum_state_duration; ///< Total duration planned for the current quantum state.

        std::vector<QPointF> trail_points;
        ///< Pre-calculated points for rendering the trail of impulse markers (type 0).
    };

    /**
     * @brief Constructs a PathMarkersManager. Initializes last update time to 0.
     */
    PathMarkersManager() : last_update_time_(0) {
    }

    /**
     * @brief Initializes or resets the set of markers.
     * Clears existing markers and creates a new random set with varying types, speeds, and appearances.
     * Ensures that the same marker type is not generated twice consecutively.
     */
    void InitializeMarkers();

    /**
     * @brief Updates the state of all markers based on the elapsed time.
     * Calculates new positions, color phases, wave phases, and quantum states.
     * @param delta_time Time elapsed since the last update in seconds.
     */
    void UpdateMarkers(double delta_time);

    /**
     * @brief Draws all managed markers onto the provided QPainter.
     * Calculates the time delta since the last draw call, updates marker states,
     * determines marker positions on the given path, and calls the appropriate
     * drawing function for each marker type. Initializes markers if the list is empty.
     * @param painter The QPainter to use for drawing.
     * @param blob_path The QPainterPath along which the markers should be drawn.
     * @param current_time The current timestamp in milliseconds (e.g., from QDateTime::currentMSecsSinceEpoch()).
     */
    void DrawMarkers(QPainter &painter, const QPainterPath &blob_path, qint64 current_time);

    /**
     * @brief Gets the current number of active markers.
     * @return The number of markers in the m_markers vector.
     */
    size_t GetMarkerCount() const {
        return markers_.size();
    }

    /**
     * @brief Gets a constant reference to a specific marker by its index.
     * Performs bounds checking via std::vector::at().
     * @param index The index of the desired marker.
     * @return A constant reference to the PathMarker struct.
     * @throws std::out_of_range if the index is invalid.
     */
    const PathMarker &GetMarker(const size_t index) const {
        return markers_.at(index);
    }

private:
    /** @brief Vector storing all the active PathMarker objects. */
    std::vector<PathMarker> markers_;
    /** @brief Timestamp (milliseconds since epoch) of the last call to drawMarkers, used for calculating deltaTime. */
    qint64 last_update_time_;

    /**
     * @brief Calculates the points needed to draw the trailing effect for an impulse marker.
     * Populates the marker's trailPoints vector.
     * @param marker Reference to the PathMarker (type 0) whose trail points are being calculated (modified).
     * @param blob_path The path along which the marker moves.
     * @param position The current absolute position (distance along the path) of the marker head.
     * @param path_length The total length of the blobPath.
     */
    static void CalculateTrailPoints(PathMarker &marker, const QPainterPath &blob_path, double position,
                                     double path_length);

    /**
     * @brief Draws a marker of type 0 (Impulse).
     * Renders a cyberpunk-style head element and a fading trail using pre-calculated points.
     * @param painter The QPainter to use for drawing.
     * @param marker The PathMarker data.
     * @param position The calculated position (QPointF) on the painter where the marker head should be drawn.
     * @param marker_color The base color for the marker.
     * @param current_time The current timestamp for potential time-based effects (e.g., flickering).
     */
    static void DrawImpulseMarker(QPainter &painter, const PathMarker &marker, const QPointF &position,
                                  const QColor &marker_color, qint64 current_time);

    /**
     * @brief Draws a marker of type 1 (Wave).
     * Renders expanding concentric rings and a subtle distortion effect originating from the marker position.
     * @param painter The QPainter to use for drawing.
     * @param marker The PathMarker data (uses wavePhase).
     * @param position The calculated position (QPointF) on the painter, representing the wave's origin.
     * @param marker_color The base color for the marker.
     */
    static void DrawWaveMarker(QPainter &painter, const PathMarker &marker, const QPointF &position,
                               const QColor &marker_color);

    /**
     * @brief Draws a marker of type 2 (Quantum).
     * Renders a central point and several orbiting "quantum copies" whose appearance and behavior
     * change based on the marker's quantumState and quantumStateTime. Includes connecting lines ("entanglement").
     * @param painter The QPainter to use for drawing.
     * @param marker The PathMarker data (uses quantumState, quantumStateTime, etc.).
     * @param position The calculated position (QPointF) on the painter for the central point.
     * @param marker_color The base color for the marker.
     * @param current_time The current timestamp for time-based animation effects.
     */
    static void DrawQuantumMarker(QPainter &painter, const PathMarker &marker, const QPointF &position,
                                  const QColor &marker_color, qint64 current_time);

    /**
     * @brief Determines the appropriate color for a marker based on its type and current color phase.
     * Uses HSV color manipulation for smooth transitions.
     * @param marker_type The type of the marker (0, 1, or 2).
     * @param color_phase The current color phase (0.0 to 1.0).
     * @return The calculated QColor for the marker.
     */
    static QColor GetMarkerColor(int marker_type, double color_phase);
};

#endif // PATH_MARKERS_MANAGER_H
