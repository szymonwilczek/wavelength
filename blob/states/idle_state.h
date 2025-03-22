#ifndef IDLESTATE_H
#define IDLESTATE_H

#include "blob_state.h"
#include "../core/blob_config.h"

class IdleState : public BlobState {
public:
    IdleState();
    
    void apply(std::vector<QPointF>& controlPoints, 
              std::vector<QPointF>& velocity,
              QPointF& blobCenter,
              const BlobConfig::BlobParameters& params) override;
              
    void applyForce(const QVector2D& force,
                   std::vector<QPointF>& velocity,
                   QPointF& blobCenter,
                   const std::vector<QPointF>& controlPoints,
                   double blobRadius) override;
    
    void updatePhases();

    void resetInitialization() {
        m_isInitializing = true;
        m_heartbeatCount = 0;
        m_heartbeatPhase = 0.0;
    }

    // Deklaracja nowej metody
    void applyHeartbeatEffect(std::vector<QPointF>& controlPoints,
                             std::vector<QPointF>& velocity,
                             QPointF& blobCenter,
                             const BlobConfig::BlobParameters& params);
    
private:
    BlobConfig::IdleParameters m_idleParams;
    double m_secondPhase = 0.0;
    double m_rotationPhase = 0.0;
    double m_mainPhaseOffset = 0.0;
    bool m_isInitializing = true;
    int m_heartbeatCount = 0;
    double m_heartbeatPhase = 0.0;
    const int REQUIRED_HEARTBEATS = 1;
};

#endif // IDLESTATE_H