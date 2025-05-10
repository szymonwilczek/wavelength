#ifndef RESIZINGSTATE_H
#define RESIZINGSTATE_H

#include "blob_state.h"

/**
 * @brief Implements the Resizing state behavior for the Blob animation.
 *
 * This state is active when the application window is being resized.
 * It primarily handles the adjustment of the blob's position and applies
 * a force based on the size change to create a visual reaction.
 * The Apply() method in this state currently does nothing, as the main logic
 * is triggered by the HandleResize() method.
 */
class ResizingState final : public BlobState {
public:
    /**
     * @brief Default constructor for ResizingState.
     */
    ResizingState();

    /**
     * @brief Applies state-specific logic. Currently empty for ResizingState.
     * The primary logic for this state is handled in HandleResize().
     * @param control_points Reference to the vector of current control point positions.
     * @param velocity Reference to the vector of current control point velocities.
     * @param blob_center Reference to the blob's center position.
     * @param params Blob appearance parameters (read-only).
     */
    void Apply(std::vector<QPointF> &control_points,
               std::vector<QPointF> &velocity,
               QPointF &blob_center,
               const BlobConfig::BlobParameters &params) override;

    /**
     * @brief Applies an external force to the blob, similar to the Moving state.
     * The force applied to each control point's velocity is scaled based on its distance from the center.
     * A portion of the force is also applied directly to the blob's center position.
     * This is used by HandleResize to apply a force based on the size change.
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
     * @brief Handles the logic when a resize event occurs while in this state.
     * Recalculates the blob's center based on the new size, shifts control points
     * and target points accordingly, and applies a force based on the magnitude
     * and direction of the size change.
     * @param control_points Reference to the vector of current control point positions (modified).
     * @param target_points Reference to the vector of target control point positions (modified).
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center Reference to the blob's center position (modified).
     * @param old_size The size of the widget before the resize.
     * @param new_size The size of the widget after the resize.
     */
    void HandleResize(std::vector<QPointF> &control_points,
                      std::vector<QPointF> &target_points,
                      std::vector<QPointF> &velocity,
                      QPointF &blob_center,
                      const QSize &old_size,
                      const QSize &new_size);
};

#endif // RESIZINGSTATE_H
