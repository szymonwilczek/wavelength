#ifndef BLOBPHYSICS_H
#define BLOBPHYSICS_H

#include <QThreadPool>
#include <vector>
#include <QVector2D>
#include "../blob_config.h"
#include "../utils/blob_math.h"

/**
 * @brief Handles the physics simulation for the Blob animation.
 *
 * This class encapsulates the logic for updating the positions and velocities
 * of the blob's control points based on physical forces (springs, damping, viscosity),
 * constraints (neighbor distances, border collisions), and shape smoothing.
 * It provides different update methods, including optimized and parallel versions.
 * It also calculates window velocity based on position changes over time.
 */
class BlobPhysics {
public:
    /**
     * @brief Constructs a BlobPhysics object.
     * Initializes the internal thread pool and starts the physics timer.
     */
    BlobPhysics();

    /**
    * @brief Initializes the blob's state vectors (control points, target points, velocity) and center position.
    * Generates initial circular points.
    * @param control_points Reference to the vector of control point positions (modified).
    * @param target_points Reference to the vector of target control point positions (modified).
    * @param velocity Reference to the vector of control point velocities (modified).
    * @param blob_center Reference to the blob's center position (modified).
    * @param params Blob appearance parameters (read-only).
    * @param width The width of the widget area.
    * @param height The height of the widget area.
    */
    static void InitializeBlob(std::vector<QPointF> &control_points,
                               std::vector<QPointF> &target_points,
                               std::vector<QPointF> &velocity,
                               QPointF &blob_center,
                               const BlobConfig::BlobParameters &params,
                               int width, int height);

    /**
     * @brief Updates the blob physics using an optimized Structure-of-Arrays (SoA) approach with OpenMP parallelization.
     * Suitable for a large number of control points. Converts data to separate float arrays for better cache performance and vectorization.
     * Includes safety checks for vector sizes.
     * @param control_points Reference to the vector of current control point positions (modified).
     * @param target_points Reference to the vector of target control point positions (read-only).
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center The current center position of the blob (read-only).
     * @param params Blob appearance parameters (read-only).
     * @param physics_params Blob physics parameters (read-only).
     */
    static void UpdatePhysicsOptimized(std::vector<QPointF> &control_points, const std::vector<QPointF> &target_points,
                                       std::vector<QPointF> &velocity, const QPointF &blob_center,
                                       const BlobConfig::BlobParameters &params,
                                       const BlobConfig::PhysicsParameters &physics_params);

    /**
     * @brief Updates the blob physics using QtConcurrent for parallel processing across multiple threads.
     * Divides the work into batches and processes them using the internal thread pool.
     * Falls back to UpdatePhysicsOptimized or UpdatePhysics for different point counts.
     * Includes safety checks for vector sizes and thread ranges.
     * @param control_points Reference to the vector of current control point positions (modified).
     * @param target_points Reference to the vector of target control point positions (read-only).
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center The current center position of the blob (read-only).
     * @param params Blob appearance parameters (read-only).
     * @param physics_params Blob physics parameters (read-only).
     */
    void UpdatePhysicsParallel(std::vector<QPointF> &control_points, std::vector<QPointF> &target_points,
                               std::vector<QPointF> &velocity, const QPointF &blob_center,
                               const BlobConfig::BlobParameters &params,
                               const BlobConfig::PhysicsParameters &physics_params);

    /**
     * @brief Updates the blob physics using a standard sequential approach.
     * Iterates through each control point, calculates forces, applies damping, updates velocity and position.
     * Includes safety checks for vector sizes.
     * @param control_points Reference to the vector of current control point positions (modified).
     * @param target_points Reference to the vector of target control point positions (read-only).
     * @param velocity Reference to the vector of current control point velocities (modified).
     * @param blob_center The current center position of the blob (read-only).
     * @param params Blob appearance parameters (read-only).
     * @param physics_params Blob physics parameters (read-only).
     */
    static void UpdatePhysics(std::vector<QPointF> &control_points,
                              const std::vector<QPointF> &target_points,
                              std::vector<QPointF> &velocity,
                              const QPointF &blob_center,
                              const BlobConfig::BlobParameters &params,
                              const BlobConfig::PhysicsParameters &physics_params);

    /**
     * @brief Handles collisions between blob control points and the widget borders.
     * Adjusts positions and reverses velocity components based on the restitution factor.
     * @param control_points Reference to the vector of control point positions (modified).
     * @param velocity Reference to the vector of control point velocities (modified).
     * @param blob_center Reference to the blob's center position (modified).
     * @param width The width of the widget area.
     * @param height The height of the widget area.
     * @param restitution The bounciness factor (0.0 to 1.0).
     * @param padding The distance from the edge where collisions occur.
     */
    static void HandleBorderCollisions(std::vector<QPointF> &control_points,
                                       std::vector<QPointF> &velocity,
                                       QPointF &blob_center,
                                       int width, int height,
                                       double restitution,
                                       int padding);

    /**
     * @brief Enforces minimum and maximum distance constraints between neighboring control points.
     * Adjusts positions and applies small velocity changes to maintain shape integrity.
     * @param control_points Reference to the vector of control point positions (modified).
     * @param velocity Reference to the vector of control point velocities (modified).
     * @param min_distance The minimum allowed distance between neighbors.
     * @param max_distance The maximum allowed distance between neighbors.
     */
    static void ConstrainNeighborDistances(std::vector<QPointF> &control_points,
                                           std::vector<QPointF> &velocity,
                                           double min_distance,
                                           double max_distance);

    /**
     * @brief Applies a smoothing filter to the blob's shape.
     * Moves each control point slightly towards the average position of its neighbors.
     * @param control_points Reference to the vector of control point positions (modified).
     */
    static void SmoothBlobShape(std::vector<QPointF> &control_points);

    /**
     * @brief Gradually moves control points towards an ideal circular/organic shape when the blob is idle.
     * Helps prevent the blob from collapsing or drifting excessively when not moving.
     * @param control_points Reference to the vector of control point positions (modified).
     * @param blob_center The current center position of the blob (read-only).
     * @param blob_radius The target average radius of the blob.
     * @param stabilization_rate The rate at which points move towards the ideal shape (0.0 to 1.0).
     */
    static void StabilizeBlob(std::vector<QPointF> &control_points,
                              const QPointF &blob_center,
                              double blob_radius,
                              double stabilization_rate);

    /**
     * @brief Checks if any control points or velocities contain invalid values (NaN, infinity).
     * If invalid points are found, reset the entire blob shape to a default organic form around the center.
     * @param control_points Reference to the vector of control point positions (modified).
     * @param velocity Reference to the vector of control point velocities (modified).
     * @param blob_center The current center position of the blob (read-only).
     * @param blob_radius The target average radius of the blob.
     * @return True if invalid points were found and repaired, false otherwise.
     */
    static bool ValidateAndRepairControlPoints(std::vector<QPointF> &control_points,
                                               std::vector<QPointF> &velocity,
                                               const QPointF &blob_center,
                                               double blob_radius);

    /**
     * @brief Calculates the approximate velocity of the window based on position changes over time.
     * Uses the internal physics_timer_ and applies smoothing. Clamps maximum velocity.
     * @param current_position The current position of the window.
     * @return The calculated window velocity as a QVector2D.
     */
    QVector2D CalculateWindowVelocity(const QPointF &current_position);

    /**
     * @brief Sets the last known position of the window. Used by CalculateWindowVelocity.
     * @param position The window position.
     */
    void SetLastWindowPos(const QPointF &position);

    /**
     * @brief Gets the last recorded window position.
     * @return The last window position as a QPointF.
     */
    [[nodiscard]] QPointF GetLastWindowPos() const;

    /**
     * @brief Gets the last calculated window velocity.
     * @return The last window velocity as a QVector2D.
     */
    [[nodiscard]] QVector2D GetLastWindowVelocity() const;

private:
    /** @brief Timer used for calculating delta time in velocity calculations. */
    QElapsedTimer physics_timer_;
    /** @brief Stores the last known window position for velocity calculation. */
    QPointF last_window_position_;
    /** @brief Stores the last calculated window velocity. */
    QVector2D last_window_velocity_;
    /** @brief Thread pool used for parallel physics updates (UpdatePhysicsParallel). */
    QThreadPool thread_pool_;
};

#endif // BLOBPHYSICS_H
