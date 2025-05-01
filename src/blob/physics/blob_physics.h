#ifndef BLOBPHYSICS_H
#define BLOBPHYSICS_H

#include <QThreadPool>
#include <QTime>
#include "../blob_config.h"
#include "../utils/blob_math.h"

class BlobPhysics {
public:
    BlobPhysics();

    static void UpdatePhysicsOptimized(std::vector<QPointF> &control_points, const std::vector<QPointF> &target_points,
                                       std::vector<QPointF> &velocity, const QPointF &blob_center,
                                       const BlobConfig::BlobParameters &params,
                                       const BlobConfig::PhysicsParameters &physics_params);

    void UpdatePhysicsParallel(std::vector<QPointF> &control_points, std::vector<QPointF> &target_points,
                               std::vector<QPointF> &velocity, const QPointF &blob_center,
                               const BlobConfig::BlobParameters &params,
                               const BlobConfig::PhysicsParameters &physics_params);

    static void UpdatePhysics(std::vector<QPointF> &control_points,
                              const std::vector<QPointF> &target_points,
                              std::vector<QPointF> &velocity,
                              const QPointF &blob_center,
                              const BlobConfig::BlobParameters &params,
                              const BlobConfig::PhysicsParameters &physics_params);

    static void InitializeBlob(std::vector<QPointF>& control_points,
                               std::vector<QPointF>& target_points,
                               std::vector<QPointF>& velocity,
                               QPointF& blob_center,
                               const BlobConfig::BlobParameters& params,
                               int width, int height);

    static void HandleBorderCollisions(std::vector<QPointF>& control_points,
                                       std::vector<QPointF>& velocity,
                                       QPointF& blob_center,
                                       int width, int height,
                                       double restitution,
                                       int padding);

    static void ConstrainNeighborDistances(std::vector<QPointF>& control_points,
                                           std::vector<QPointF>& velocity,
                                           double min_distance,
                                           double max_distance);

    static void SmoothBlobShape(std::vector<QPointF>& control_points);

    static void StabilizeBlob(std::vector<QPointF>& control_points,
                              const QPointF& blob_center,
                              double blob_radius,
                              double stabilization_rate);

    static bool ValidateAndRepairControlPoints(std::vector<QPointF>& control_points,
                                               std::vector<QPointF>& velocity,
                                               const QPointF& blob_center,
                                               double blob_radius);

    QVector2D CalculateWindowVelocity(const QPointF& current_position);

    void SetLastWindowPos(const QPointF& position);


    QPointF GetLastWindowPos() const;
    QVector2D GetLastWindowVelocity() const;

private:
    QTime physics_timer_;
    QPointF last_window_position_;
    QVector2D last_window_velocity_;
    QThreadPool thread_pool_;
};

#endif // BLOBPHYSICS_H