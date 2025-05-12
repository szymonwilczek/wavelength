#ifndef IDLESTATE_H
#define IDLESTATE_H

#include "blob_state.h"

/**
 * @brief Implements the Idle state behavior for the Blob animation.
 *
 * This state applies subtle wave and rotation effects to the blob when it's not actively
 * moving or resizing. It also includes an initial "heartbeat" animation upon entering the state.
 * Forces applied in this state are dampened compared to other states.
 */
class IdleState final : public BlobState {
public:
    /**
     * @brief Constructs an IdleState object.
     * Initializes phase offsets.
     */
    IdleState();

    /**
     * @brief Applies the idle animation effects (waves, rotation, centering) or the initial heartbeat effect.
     * Called periodically by the animation loop when this state is active.
     * If is_initializing_ is true, applies the heartbeat effect. Otherwise, applies the standard idle wave and rotation.
     * @param control_points Reference to the vector of current control point positions (modified).
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center Reference to the blob's center position (modified).
     * @param params Blob appearance parameters (read-only).
     */
    void Apply(std::vector<QPointF> &control_points,
               std::vector<QPointF> &velocity,
               QPointF &blob_center,
               const BlobConfig::BlobParameters &params) override;

    /**
     * @brief Applies an external force to the blob with dampening specific to the Idle state.
     * The force is scaled based on the distance of each control point from the center and applied primarily to velocity.
     * A small portion of the force is also applied directly to the blob's center position.
     * @param force The external force vector to apply.
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center Reference to the blob's center position (modified).
     * @param control_points Reference to the vector of current control point positions (read-only).
     * @param blob_radius The current average radius of the blob (read-only).
     */
    void ApplyForce(const QVector2D &force,
                    std::vector<QPointF> &velocity,
                    QPointF &blob_center,
                    const std::vector<QPointF> &control_points,
                    double blob_radius) override;

    /**
     * @brief Updates the internal phase variables used for the wave and rotation animations.
     * Increments wave_phase, second_phase_, and rotation_phase_, wrapping them around 2*PI.
     */
    void UpdatePhases();

    /**
     * @brief Resets the initialization state, causing the heartbeat effect to play again.
     * Sets is_initializing_ to true and resets heartbeat counters.
     */
    void ResetInitialization();

    /**
     * @brief Applies a pulsating "heartbeat" effect to the blob.
     * Used during the initial phase after entering the Idle state. The effect runs for a defined number of beats (kRequiredHeartbeats).
     * Modifies control point velocities to create expansion and contraction.
     * @param control_points Reference to the vector of current control point positions (read-only).
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center The current center position of the blob (read-only).
     * @param params Blob appearance parameters (read-only).
     */
    void ApplyHeartbeatEffect(const std::vector<QPointF> &control_points,
                              std::vector<QPointF> &velocity,
                              const QPointF &blob_center,
                              const BlobConfig::BlobParameters &params);

private:
    /** @brief Parameters specific to the idle animation (wave amplitude, frequency, phase). */
    BlobConfig::IdleParameters idle_params_;
    /** @brief Phase offset for the secondary wave effect. */
    double second_phase_ = 0.0;
    /** @brief Phase offset for the rotational effect. */
    double rotation_phase_ = 0.0;
    /** @brief Flag indicating if the state is currently running the initial heartbeat animation. */
    bool is_initializing_ = true;
    /** @brief Counter for the number of heartbeats completed during initialization. */
    int heartbeat_count_ = 0;
    /** @brief Current phase of the heartbeat animation cycle (0 to 2*PI). */
    double heartbeat_phase_ = 0.0;
    /** @brief The number of heartbeat cycles to perform during initialization. */
    static constexpr int kRequiredHeartbeats = 1;
};

#endif // IDLESTATE_H
