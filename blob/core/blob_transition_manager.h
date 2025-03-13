#ifndef BLOB_TRANSITION_MANAGER_H
#define BLOB_TRANSITION_MANAGER_H

#include <deque>
#include <QVector2D>
#include <QPointF>
#include <QTimer>
#include <QDateTime>
#include <vector>
#include <functional>
#include "blob_config.h"

class BlobTransitionManager : public QObject {
    Q_OBJECT

public:
    explicit BlobTransitionManager(QObject* parent = nullptr);
    ~BlobTransitionManager() override;

    struct WindowMovementSample {
        QPointF position;
        qint64 timestamp;
    };

    void startTransitionToIdle(
        const std::vector<QPointF>& currentControlPoints, 
        const std::vector<QPointF>& currentVelocity,
        const QPointF& currentBlobCenter,
        const QPointF& targetCenter,
        int width, 
        int height
    );

    void handleIdleTransition(
        std::vector<QPointF>& controlPoints,
        std::vector<QPointF>& velocity,
        QPointF& blobCenter,
        const BlobConfig::BlobParameters& params,
        std::function<void(std::vector<QPointF>&, std::vector<QPointF>&, QPointF&, const BlobConfig::BlobParameters&)> applyIdleState
    );

    bool isInTransitionToIdle() const { return m_inTransitionToIdle; }
    
    void processMovementBuffer(
        std::vector<QPointF>& velocity,
        QPointF& blobCenter,
        std::vector<QPointF>& controlPoints,
        float blobRadius,
        std::function<void(std::vector<QPointF>&, QPointF&, std::vector<QPointF>&, float, QVector2D)> applyInertiaForce,
        std::function<void(const QPointF&)> setLastWindowPos
    );

    void addMovementSample(const QPointF& position, qint64 timestamp);
    void clearMovementBuffer();
    
    bool isMoving() const { return m_isMoving; }
    void setMoving(bool isMoving) { m_isMoving = isMoving; }
    
    void resetInactivityCounter() { m_inactivityCounter = 0; }
    int getInactivityCounter() const { return m_inactivityCounter; }
    void incrementInactivityCounter() { m_inactivityCounter++; }

    qint64 getLastMovementTime() const { return m_lastMovementTime; }
    void setLastMovementTime(qint64 time) { m_lastMovementTime = time; }

    std::pmr::deque<WindowMovementSample> getMovementBuffer() const { return m_movementBuffer; }

signals:
    void transitionCompleted();
    void significantMovementDetected();
    void movementStopped();

private:
    static const int MAX_MOVEMENT_SAMPLES = 10;
    std::pmr::deque<WindowMovementSample> m_movementBuffer;
    QVector2D m_smoothedVelocity;

    bool m_inTransitionToIdle = false;
    qint64 m_transitionToIdleStartTime = 0;
    qint64 m_transitionToIdleDuration = 0;
    std::vector<QPointF> m_originalControlPoints;
    std::vector<QPointF> m_originalVelocities;
    QPointF m_originalBlobCenter;
    std::vector<QPointF> m_targetIdlePoints;
    QPointF m_targetIdleCenter;
    QTimer* m_transitionToIdleTimer = nullptr;

    bool m_isMoving = false;
    int m_inactivityCounter = 0;
    qint64 m_lastMovementTime = 0;
};

#endif // BLOB_TRANSITION_MANAGER_H