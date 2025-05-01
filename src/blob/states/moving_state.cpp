#include "moving_state.h"
#include <QDebug>

MovingState::MovingState() = default;

void MovingState::Apply(std::vector<QPointF>& control_points,
                       std::vector<QPointF>& velocity,
                       QPointF& blob_center,
                       const BlobConfig::BlobParameters& params) {
    double avg_velocity_x = 0.0;
    double avg_velocity_y = 0.0;

    for (const auto& vel : velocity) {
        avg_velocity_x += vel.x();
        avg_velocity_y += vel.y();
    }
    avg_velocity_x /= velocity.size();
    avg_velocity_y /= velocity.size();

    if (std::abs(avg_velocity_x) > 0.1 || std::abs(avg_velocity_y) > 0.1) {
        QVector2D avg_direction(avg_velocity_x, avg_velocity_y);
        avg_direction.normalize();

        for (size_t i = 0; i < control_points.size(); ++i) {
            QPointF vector_from_center = control_points[i] - blob_center;
            const double distance_from_center = QVector2D(vector_from_center).length();

            double factor = distance_from_center / params.blob_radius;
            // Zmniejsz współczynnik odkształcenia (z 0.1 na 0.05)
            factor = factor * factor * 0.05;

            auto stretch_force = QPointF(avg_velocity_x * factor, avg_velocity_y * factor);
            // Zmniejsz wpływ siły (z 0.025 na 0.015)
            velocity[i] += stretch_force * 0.015;
        }
    }
}

void MovingState::ApplyInertiaForce(std::vector<QPointF>& velocity,
                                  QPointF& blob_center,
                                  const std::vector<QPointF>& control_points,
                                  const double blob_radius,
                                  const QVector2D& window_velocity) {
    // Unikamy kosztownych obliczeń dla bardzo małych prędkości
    const double window_speed = window_velocity.length();
    if (window_speed < 0.1) {
        return;
    }

    // Zmniejsz współczynnik siły bezwładności z 0.005 na 0.0025
    const QVector2D inertia_force = -window_velocity * (0.0025 * qMin(1.0, window_speed / 10.0));

    // Prekalkujemy dla wydajności
    const double force_x = inertia_force.x();
    const double force_y = inertia_force.y();

    // Prekalkujemy środek
    const double center_x = blob_center.x();
    const double center_y = blob_center.y();

    // Prekalkujemy odwrotność promienia
    const double inverted_radius = 1.0 / blob_radius;

    // Zmniejsz wpływ sił na punkty kontrolne (z 0.4 na 0.2)
    for (size_t i = 0; i < control_points.size(); ++i) {
        double distance_x = control_points[i].x() - center_x;
        double distance_y = control_points[i].y() - center_y;

        const double distance_approx = qAbs(distance_x) + qAbs(distance_y);
        const double force_scale = qMin(distance_approx * inverted_radius * 0.2, 0.3); // Zmniejszono oba współczynniki

        velocity[i].rx() += force_x * force_scale;
        velocity[i].ry() += force_y * force_scale;
    }

    // Zmniejsz wpływ na środek bloba (z 0.0005 na 0.0003)
    const double center_factor = 0.0003 * qMin(1.0, window_speed / 5.0);
    blob_center.rx() += -window_velocity.x() * center_factor;
    blob_center.ry() += -window_velocity.y() * center_factor;

    // Zmniejsz współczynnik dodatkowych sił dla gwałtownych ruchów
    if (window_speed > 5.0) {
        for (size_t i = 0; i < control_points.size(); ++i) {
            const double distance_x = control_points[i].x() - center_x;
            const double distance_y = control_points[i].y() - center_y;

            const double perp_x = -force_y;
            const double perp_y = force_x;

            const double dot_product = distance_x * perp_x + distance_y * perp_y;
            // Zmniejszono z 0.00005 na 0.00002
            velocity[i].rx() += dot_product * 0.00002;
            velocity[i].ry() += dot_product * 0.00002;
        }
    }
}

void MovingState::ApplyForce(const QVector2D& force,
                           std::vector<QPointF>& velocity,
                           QPointF& blob_center,
                           const std::vector<QPointF>& control_points,
                           const double blob_radius) {
    
    const size_t num_of_points = control_points.size();
    static std::vector<double> distance_cache;

    if (distance_cache.size() != num_of_points) {
        distance_cache.resize(num_of_points);
    }

    for (size_t i = 0; i < num_of_points; ++i) {
        QPointF vector_from_center = control_points[i] - blob_center;
        distance_cache[i] = QVector2D(vector_from_center).length();
    }

    const QPointF force_xy(force.x(), force.y());
    const QPointF center_force = force_xy * 0.2;
    blob_center += center_force;

    for (size_t i = 0; i < num_of_points; ++i) {
        const double force_scale = qMin(distance_cache[i] / blob_radius, 1.0);
        velocity[i] += force_xy * (force_scale * 0.8);
    }
}