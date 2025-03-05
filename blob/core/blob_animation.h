#ifndef BLOBANIMATION_H
#define BLOBANIMATION_H

#include <QWidget>
#include <QTimer>
#include <QTime>
#include <QPointF>
#include <QColor>
#include <QResizeEvent>
#include <QEvent>
#include <vector>
#include <memory>

#include "blob_config.h"
#include "../physics/blob_physics.h"
#include "../rendering/blob_render.h"
#include "../states/blob_state.h"
#include "../states/idle_state.h"
#include "../states/moving_state.h"
#include "../states/resizing_state.h"

class BlobAnimation : public QWidget {
    Q_OBJECT

public:
    explicit BlobAnimation(QWidget *parent = nullptr);
    ~BlobAnimation() override;

    void setBackgroundColor(const QColor &color);

    void setGridColor(const QColor &color);

    void setGridSpacing(int spacing);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void updateAnimation();

private:
    void initializeBlob();

    void switchToState(BlobConfig::AnimationState newState);

    void applyForces(const QVector2D &force);

    void applyIdleEffect();

    BlobConfig::BlobParameters m_params;
    BlobConfig::PhysicsParameters m_physicsParams;
    BlobConfig::IdleParameters m_idleParams;

    std::vector<QPointF> m_controlPoints;
    std::vector<QPointF> m_targetPoints;
    std::vector<QPointF> m_velocity;

    QPointF m_blobCenter;

    BlobConfig::AnimationState m_currentState = BlobConfig::IDLE;

    QTimer m_animationTimer;
    QTimer m_idleTimer;
    QTimer m_stateResetTimer;
    QTimer* m_transitionToIdleTimer = nullptr;

    BlobPhysics m_physics;
    BlobRenderer m_renderer;

    std::unique_ptr<IdleState> m_idleState;
    std::unique_ptr<MovingState> m_movingState;
    std::unique_ptr<ResizingState> m_resizingState;
    BlobState* m_currentBlobState;

    QPointF m_lastWindowPos;
    QTimer* m_windowPosCheckTimer = nullptr;

    bool m_inTransitionToIdle = false;
    qint64 m_transitionToIdleStartTime = 0;
    qint64 m_transitionToIdleDuration = 0;
    std::vector<QPointF> m_originalControlPoints;
    std::vector<QPointF> m_originalVelocities;
    QPointF m_originalBlobCenter;
    std::vector<QPointF> m_targetIdlePoints;
    QPointF m_targetIdleCenter;
};

#endif // BLOBANIMATION_H