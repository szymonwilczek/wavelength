#include "resizing_state.h"

#include <QSizeF>

ResizingState::ResizingState() {
}

void ResizingState::Apply(std::vector<QPointF> &control_points,
                          std::vector<QPointF> &velocity,
                          QPointF &blob_center,
                          const BlobConfig::BlobParameters &params) {
}

void ResizingState::HandleResize(std::vector<QPointF> &control_points,
                                 std::vector<QPointF> &target_points,
                                 std::vector<QPointF> &velocity,
                                 QPointF &blob_center,
                                 const QSize &old_size,
                                 const QSize &new_size) {
    const QPointF old_center = blob_center;

    blob_center = QPointF(new_size.width() / 2.0, new_size.height() / 2.0);

    const QPointF delta = blob_center - old_center;

    for (size_t i = 0; i < control_points.size(); ++i) {
        control_points[i] += delta;
        target_points[i] += delta;
    }

    if (old_size.isValid()) {
        const QVector2D resize_force(
            (new_size.width() - old_size.width()) * 0.05,
            (new_size.height() - old_size.height()) * 0.05
        );

        ApplyForce(resize_force, velocity, blob_center, control_points, old_size.width() / 2.0);
    }
}

void ResizingState::ApplyForce(const QVector2D &force,
                               std::vector<QPointF> &velocity,
                               QPointF &blob_center,
                               const std::vector<QPointF> &control_points,
                               const double blob_radius) {
    for (size_t i = 0; i < control_points.size(); ++i) {
        QPointF vector_from_center = control_points[i] - blob_center;
        const double distance_from_center = QVector2D(vector_from_center).length();
        double force_scale = distance_from_center / blob_radius;

        if (force_scale > 1.0) force_scale = 1.0;

        velocity[i] += QPointF(
            force.x() * force_scale * 0.8,
            force.y() * force_scale * 0.8
        );
    }

    blob_center += QPointF(force.x() * 0.2, force.y() * 0.2);
}
