#ifndef IDLESTATE_H
#define IDLESTATE_H

#include <QVector2D>

#include "blob_state.h"
#include "../blob_config.h"

class IdleState final : public BlobState {
public:
    IdleState();
    
    void Apply(std::vector<QPointF>& control_points,
              std::vector<QPointF>& velocity,
              QPointF& blob_center,
              const BlobConfig::BlobParameters& params) override;
              
    void ApplyForce(const QVector2D& force,
                   std::vector<QPointF>& velocity,
                   QPointF& blob_center,
                   const std::vector<QPointF>& control_points,
                   double blob_radius) override;
    
    void UpdatePhases();

    void ResetInitialization();

    void ApplyHeartbeatEffect(const std::vector<QPointF>& control_points,
                             std::vector<QPointF>& velocity,
                             const QPointF& blob_center,
                             const BlobConfig::BlobParameters& params);
    
private:
    BlobConfig::IdleParameters idle_params_;
    double second_phase_ = 0.0;
    double rotation_phase_ = 0.0;
    double main_phase_offset_ = 0.0;
    bool is_initializing_ = true;
    int heartbeat_count_ = 0;
    double heartbeat_phase_ = 0.0;
    const int kRequiredHeartbeats = 1;
};

#endif // IDLESTATE_H