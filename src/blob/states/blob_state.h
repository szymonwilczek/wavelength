#ifndef BLOBSTATE_H
#define BLOBSTATE_H

#include <QVector2D>
#include <vector>
#include "../blob_config.h"

/**
 * @brief Abstract base class defining the interface for different blob animation states.
 *
 * This class serves as the foundation for specific state implementations (e.g., Idle, Moving, Resizing).
 * Each derived class must implement the Apply() and ApplyForce() methods to define the
 * behavior and physics modifications specific to that state.
 */
class BlobState {
public:
    /**
     * @brief Virtual destructor. Ensures proper cleanup when deleting derived state objects through a base class pointer.
     */
    virtual ~BlobState() = default;

    /**
     * @brief Applies the state-specific logic and effects to the blob.
     * This method is called periodically by the animation loop to update the blob's appearance
     * or behavior based on the current state (e.g., applying idle wave effect).
     * @param control_points Reference to the vector of current control point positions (can be modified).
     * @param velocity Reference to the vector of current control point velocities (can be modified).
     * @param blob_center Reference to the blob's center position (can be modified).
     * @param params Blob appearance parameters (read-only).
     */
    virtual void Apply(std::vector<QPointF> &control_points,
                       std::vector<QPointF> &velocity,
                       QPointF &blob_center,
                       const BlobConfig::BlobParameters &params) = 0;

    /**
     * @brief Applies an external force to the blob, potentially modified by the current state.
     * Allows states to react differently to external forces (e.g., inertia from window movement).
     * @param force The external force vector to apply.
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center Reference to the blob's center position (modified).
     * @param control_points Reference to the vector of current control point positions (read-only, used for calculations).
     * @param blob_radius The current average radius of the blob (read-only, used for calculations).
     */
    virtual void ApplyForce(const QVector2D &force,
                            std::vector<QPointF> &velocity,
                            QPointF &blob_center,
                            const std::vector<QPointF> &control_points,
                            double blob_radius) = 0;
};

#endif // BLOBSTATE_H
