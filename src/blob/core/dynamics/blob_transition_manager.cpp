#include "blob_transition_manager.h"

#include <QDateTime>

BlobTransitionManager::BlobTransitionManager(QObject *parent)
    : QObject(parent) {
}

void BlobTransitionManager::ProcessMovementBuffer(
    std::vector<QPointF> &velocity,
    QPointF &blob_center,
    std::vector<QPointF> &control_points,
    const float blob_radius,
    const std::function<void(std::vector<QPointF> &, QPointF &, std::vector<QPointF> &, float, QVector2D)> &
    ApplyInertiaForce,
    const std::function<void(const QPointF &)> &SetLastWindowPos) {
    if (is_resizing_) {
        return;
    }

    const qint64 current_time = QDateTime::currentMSecsSinceEpoch();

    if (movement_buffer_.size() < 2)
        return;

    QVector2D new_velocity(0, 0);
    double total_weight = 0;

    // calculating movement buffer based on movement samples
    for (size_t i = 1; i < movement_buffer_.size(); i++) {
        QPointF delta_position = movement_buffer_[i].position - movement_buffer_[i - 1].position;

        if (const qint64 delta_time = movement_buffer_[i].timestamp - movement_buffer_[i - 1].timestamp;
            delta_time > 0) {
            const double scale = 700.0 / delta_time;
            QVector2D sample_velocity(delta_position.x() * scale, delta_position.y() * scale);
            const double weight = 0.5 + 0.5 * (i / static_cast<double>(movement_buffer_.size() - 1));
            new_velocity += sample_velocity * weight;
            total_weight += weight;
        }
    }

    if (total_weight > 0) {
        new_velocity /= total_weight;
    }

    // smoothing velocity
    if (m_smoothed_velocity_.length() > 0) {
        m_smoothed_velocity_ = m_smoothed_velocity_ * 0.7 + new_velocity * 0.2;
    } else {
        m_smoothed_velocity_ = new_velocity;
    }

    const double velocity_magnitude = m_smoothed_velocity_.length();

    if (const bool significant_movement = velocity_magnitude > 0.3;
        significant_movement || current_time - last_movement_time_ < 200) {
        if (significant_movement) {
            const QVector2D scaled_velocity = m_smoothed_velocity_ * 0.6;
            ApplyInertiaForce(
                velocity,
                blob_center,
                control_points,
                blob_radius,
                scaled_velocity
            );

            SetLastWindowPos(movement_buffer_.back().position);

            is_moving_ = true;
            inactivity_counter_ = 0;

            emit significantMovementDetected();
        }
    } else if (is_moving_) {
        inactivity_counter_++;

        if (inactivity_counter_ > 60) {
            is_moving_ = false;
            emit movementStopped();
        }
    }

    // clearing buffer after idle time
    if (movement_buffer_.size() > 1 && current_time - last_movement_time_ > 500) {
        const QPointF last_position = movement_buffer_.back().position;
        const qint64 last_timestamp = movement_buffer_.back().timestamp;

        movement_buffer_.clear();
        movement_buffer_.push_back({last_position, last_timestamp});
    }
}

void BlobTransitionManager::AddMovementSample(const QPointF &position, const qint64 timestamp) {
    movement_buffer_.push_back({position, timestamp});
    if (movement_buffer_.size() > kMaxMovementSamples) {
        movement_buffer_.pop_front();
    }
    last_movement_time_ = timestamp;
}

void BlobTransitionManager::ClearMovementBuffer() {
    movement_buffer_.clear();
}
