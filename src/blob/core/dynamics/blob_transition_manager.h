#ifndef BLOB_TRANSITION_MANAGER_H
#define BLOB_TRANSITION_MANAGER_H

#include <deque>
#include <QObject>
#include <QVector2D>

/**
 * @brief Manages transitions and movement analysis for the Blob animation based on window events.
 *
 * This class processes a buffer of window movement samples to calculate velocity and determine
 * if the window is actively moving. It applies inertia forces to the blob based on this movement
 * and detects when movement starts and stops. It also handles transitions between states,
 * although the idle transition logic seems partially implemented or deprecated.
 */
class BlobTransitionManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs a BlobTransitionManager.
     * @param parent Optional parent QObject.
     */
    explicit BlobTransitionManager(QObject *parent = nullptr);

    /**
     * @brief Structure representing a single sample of the window's position at a specific time.
     */
    struct WindowMovementSample {
        QPointF position; ///< The window's top-left position at the time of the sample.
        qint64 timestamp; ///< The timestamp (milliseconds since epoch) when the sample was taken.
    };

    /**
     * @brief Processes the buffered window movement samples to calculate velocity and apply inertia.
     *
     * Calculates a smoothed velocity based on recent movement samples. If significant movement
     * is detected, it calls the provided ApplyInertiaForce function to affect the blob's dynamics,
     * updates the last known window position, resets the inactivity counter, and emits
     * significantMovementDetected(). If movement stops, it increments the inactivity counter
     * and emits movementStopped() after a period of inactivity.
     *
     * @param velocity Reference to the vector storing velocities of blob control points (modified by ApplyInertiaForce).
     * @param blob_center Reference to the blob's center position (modified by ApplyInertiaForce).
     * @param control_points Reference to the vector storing blob control points (modified by ApplyInertiaForce).
     * @param blob_radius The current radius of the blob.
     * @param ApplyInertiaForce A function callback to apply the calculated inertia force to the blob's state.
     * @param SetLastWindowPos A function callback to update the last known window position used by dynamics.
     */
    void ProcessMovementBuffer(
        std::vector<QPointF> &velocity,
        QPointF &blob_center,
        std::vector<QPointF> &control_points,
        float blob_radius,
        const std::function<void(std::vector<QPointF> &, QPointF &, std::vector<QPointF> &, float, QVector2D)> &
        ApplyInertiaForce,
        const std::function<void(const QPointF &)> &SetLastWindowPos
    );

    /**
     * @brief Clears the internal movement sample buffer.
     * @deprecated Consider using ClearMovementBuffer() instead for clarity.
     */
    void ClearAllMovementBuffers() {
        movement_buffer_.clear();
    }

    /**
     * @brief Adds a new window movement sample to the internal buffer.
     * Removes the oldest sample if the buffer exceeds kMaxMovementSamples. Updates last_movement_time_.
     * @param position The window position sample.
     * @param timestamp The timestamp of the sample.
     */
    void AddMovementSample(const QPointF &position, qint64 timestamp);

    /**
     * @brief Clears the internal buffer of window movement samples.
     */
    void ClearMovementBuffer();

    /**
     * @brief Checks if the manager currently considers the window to be moving.
     * @return True if moving, false otherwise.
     */
    [[nodiscard]] bool IsMoving() const { return is_moving_; }

    /**
     * @brief Manually sets the moving state.
     * @param is_moving The new moving state.
     */
    void SetMoving(const bool is_moving) { is_moving_ = is_moving; }

    /**
     * @brief Sets the flag indicating whether the window is currently being resized.
     * Movement processing is skipped if this flag is true.
     * @param is_resizing True if resizing, false otherwise.
     */
    void SetResizingState(const bool is_resizing) { is_resizing_ = is_resizing; }

    /**
     * @brief Resets the inactivity counter to zero. Typically called when movement is detected.
     */
    void ResetInactivityCounter() { inactivity_counter_ = 0; }

    /**
     * @brief Gets the current value of the inactivity counter.
     * @return The inactivity counter-value.
     */
    [[nodiscard]] int GetInactivityCounter() const { return inactivity_counter_; }

    /**
     * @brief Increments the inactivity counter. Called when no significant movement is detected.
     */
    void IncrementInactivityCounter() { inactivity_counter_++; }

    /**
     * @brief Gets the timestamp of the last recorded movement sample.
     * @return Timestamp in milliseconds since the epoch.
     */
    [[nodiscard]] qint64 GetLastMovementTime() const { return last_movement_time_; }

    /**
     * @brief Manually sets the timestamp of the last movement.
     * @param time The timestamp in milliseconds since the epoch.
     */
    void SetLastMovementTime(const qint64 time) { last_movement_time_ = time; }

    /**
     * @brief Gets a copy of the current movement buffer.
     * @return A deque containing the recent WindowMovementSample structs.
     */
    [[nodiscard]] std::pmr::deque<WindowMovementSample> GetMovementBuffer() const { return movement_buffer_; }

signals:
    /**
     * @brief Emitted when a transition (e.g., to idle state) completes. (Currently seems unused).
     */
    void transitionCompleted();

    /**
     * @brief Emitted when the calculated velocity exceeds a threshold, indicating significant window movement.
     */
    void significantMovementDetected();

    /**
     * @brief Emitted when the inactivity counter exceeds a threshold, indicating that window movement has stopped.
     */
    void movementStopped();

private:
    /** @brief Maximum number of window movement samples to store in the buffer. */
    static constexpr int kMaxMovementSamples = 10;
    /** @brief Buffer storing recent window movement samples. Uses polymorphic allocator for potential performance benefits. */
    std::pmr::deque<WindowMovementSample> movement_buffer_;
    /** @brief Smoothed velocity vector calculated from the movement buffer. */
    QVector2D m_smoothed_velocity_;
    /** @brief Flag indicating if the window is currently considered to be moving based on processed samples. */
    bool is_moving_ = false;
    /** @brief Counter incremented each frame when no significant movement is detected. Used to determine when movement stops. */
    int inactivity_counter_ = 0;
    /** @brief Timestamp (ms since epoch) of the last added movement sample. */
    qint64 last_movement_time_ = 0;
    /** @brief Flag indicating if the window is currently being resized (prevents movement processing during resize). */
    bool is_resizing_ = false;
};

#endif // BLOB_TRANSITION_MANAGER_H
