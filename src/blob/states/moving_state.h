#ifndef MOVINGSTATE_H
#define MOVINGSTATE_H

#include "blob_state.h"
#include "../blob_config.h"

/**
 * @brief Implements the Moving state behavior for the Blob animation.
 *
 * This state is active when the application window is being moved. It applies
 * forces to the blob based on the window's velocity (inertia) and also introduces
 * a stretching effect in the direction of the blob's internal average velocity.
 * External forces are applied more directly compared to the Idle state.
 */
class MovingState final : public BlobState {
public:
    /**
     * @brief Default constructor for MovingState.
     */
    MovingState();

    /**
     * @brief Applies a stretching effect to the blob based on its average internal velocity.
     * Calculates the average velocity of all control points and applies a force to each point
     * in that direction, scaled by the point's distance from the center. This creates a visual
     * stretch or "smear" effect when the blob is moving internally.
     * @param control_points Reference to the vector of current control point positions (read-only).
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center The current center position of the blob (read-only).
     * @param params Blob appearance parameters (read-only, used for radius).
     */
    void Apply(std::vector<QPointF> &control_points,
               std::vector<QPointF> &velocity,
               QPointF &blob_center,
               const BlobConfig::BlobParameters &params) override;

    /**
     * @brief Applies an external force directly to the blob's velocity and center.
     * The force applied to each control point's velocity is scaled based on its distance from the center.
     * A portion of the force is also applied directly to the blob's center position.
     * @param force The external force vector to apply.
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center Reference to the blob's center position (modified).
     * @param control_points Reference to the vector of current control point positions (read-only, used for distance calculation).
     * @param blob_radius The current average radius of the blob (read-only, used for scaling).
     */
    void ApplyForce(const QVector2D &force,
                    std::vector<QPointF> &velocity,
                    QPointF &blob_center,
                    const std::vector<QPointF> &control_points,
                    double blob_radius) override;

    /**
     * @brief Applies an inertia force based on the calculated window velocity.
     * This static method is typically called by the BlobTransitionManager when significant window movement is detected.
     * It applies a force opposite to the window's velocity to the blob's control points and center,
     * simulating inertia. The force magnitude is scaled based on window speed and point distance from the center.
     * Includes optimizations like pre-calculation and early exit for low speeds.
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center Reference to the blob's center position (modified).
     * @param control_points Reference to the vector of current control point positions (read-only).
     * @param blob_radius The current average radius of the blob (read-only).
     * @param window_velocity The calculated velocity vector of the application window.
     */
    static void ApplyInertiaForce(std::vector<QPointF> &velocity,
                                  QPointF &blob_center,
                                  const std::vector<QPointF> &control_points,
                                  double blob_radius,
                                  const QVector2D &window_velocity);
};

#endif // MOVINGSTATE_H
