#ifndef BLOB_TRANSITION_MANAGER_H
#define BLOB_TRANSITION_MANAGER_H

#include <deque>
#include <QVector2D>
#include <QPointF>
#include <QTimer>
#include <QDateTime>
#include <vector>
#include <functional>
#include "../../blob_config.h"

class BlobTransitionManager final : public QObject {
    Q_OBJECT

public:
    explicit BlobTransitionManager(QObject* parent = nullptr);
    ~BlobTransitionManager() override;

    struct WindowMovementSample {
        QPointF position;
        qint64 timestamp;
    };

    void ProcessMovementBuffer(
        std::vector<QPointF>& velocity,
        QPointF& blob_center,
        std::vector<QPointF>& control_points,
        float blob_radius,
        const std::function<void(std::vector<QPointF>&, QPointF&, std::vector<QPointF>&, float, QVector2D)> &ApplyInertiaForce,
        const std::function<void(const QPointF&)> &SetLastWindowPos
    );

    void ClearAllMovementBuffers() {
        movement_buffer_.clear();
    }

    void AddMovementSample(const QPointF& position, qint64 timestamp);
    void ClearMovementBuffer();
    
    bool IsMoving() const { return is_moving_; }
    void SetMoving(const bool is_moving) { is_moving_ = is_moving; }
    void SetResizingState(const bool is_resizing) { is_resizing_ = is_resizing; }
    
    void ResetInactivityCounter() { inactivity_counter_ = 0; }
    int GetInactivityCounter() const { return inactivity_counter_; }
    void IncrementInactivityCounter() { inactivity_counter_++; }

    qint64 GetLastMovementTime() const { return last_movement_time_; }
    void SetLastMovementTime(const qint64 time) { last_movement_time_ = time; }

    std::pmr::deque<WindowMovementSample> GetMovementBuffer() const { return movement_buffer_; }

signals:
    void transitionCompleted();
    void significantMovementDetected();
    void movementStopped();

private:
    static constexpr int kMaxMovementSamples = 10;
    std::pmr::deque<WindowMovementSample> movement_buffer_;
    QVector2D m_smoothed_velocity_;

    bool in_transition_to_idle_ = false;
    qint64 transition_to_idle_start_time_ = 0;
    qint64 transition_to_idle_duration_ = 0;
    std::vector<QPointF> original_control_points_;
    std::vector<QPointF> original_velocities_;
    QPointF original_blob_center_;
    std::vector<QPointF> target_idle_points_;
    QPointF target_idle_center_;
    QTimer* transition_to_idle_timer_ = nullptr;

    bool is_moving_ = false;
    int inactivity_counter_ = 0;
    qint64 last_movement_time_ = 0;
    bool is_resizing_ = false;
};

#endif // BLOB_TRANSITION_MANAGER_H