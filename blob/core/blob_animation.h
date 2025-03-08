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
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QPropertyAnimation>

#include "blob_config.h"
#include "../physics/blob_physics.h"
#include "../rendering/blob_render.h"
#include "../states/blob_state.h"
#include "../states/idle_state.h"
#include "../states/moving_state.h"
#include "../states/resizing_state.h"

class BlobAnimation : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float absorptionScale READ absorptionScale WRITE setAbsorptionScale)
    Q_PROPERTY(float absorptionOpacity READ absorptionOpacity WRITE setAbsorptionOpacity)
    Q_PROPERTY(float absorptionPulse READ getAbsorptionPulse WRITE setAbsorptionPulse)

public:
    explicit BlobAnimation(QWidget *parent = nullptr);

    void handleResizeTimeout();

    void checkWindowPosition();

    ~BlobAnimation() override;

    void setBackgroundColor(const QColor &color);

    void setGridColor(const QColor &color);

    void setGridSpacing(int spacing);

    float absorptionScale() const { return m_absorptionScale; }
    float absorptionOpacity() const { return m_absorptionOpacity; }

    float getAbsorptionPulse() const { return m_absorptionPulse; }
    void setAbsorptionPulse(float value) { m_absorptionPulse = value; update(); }

     void setAbsorptionScale(float scale) {
        m_absorptionScale = scale;
        update();
    }

    void setAbsorptionOpacity(float opacity) {
        m_absorptionOpacity = opacity;
        update();
    }

    QPointF getBlobCenter() const {
        if (m_controlPoints.empty()) {
            return QPointF(width() / 2.0, height() / 2.0);
        }

        return m_blobCenter;
    }

    void applyExternalForce(const QVector2D& force) {
        qDebug() << "Stosowanie siły zewnętrznej do bloba:" << force;
        // TODO
    }

    void startBeingAbsorbed();
    void finishBeingAbsorbed();
    void cancelAbsorption();

    void startAbsorbing(const QString& targetId);
    void finishAbsorbing(const QString& targetId);
    void cancelAbsorbing(const QString& targetId);

    bool isBeingAbsorbed() const { return m_isBeingAbsorbed; }
    bool isAbsorbing() const { return m_isAbsorbing; }

    void updateAbsorptionProgress(float progress);

protected:
    void paintEvent(QPaintEvent *event) override;

    QRectF calculateBlobBoundingRect();

    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void updateAnimation();

    void updatePhysics();

    void handleIdleTransition();

    void onStateResetTimeout();

private:
    void initializeBlob();

    void switchToState(BlobConfig::AnimationState newState);

    void applyForces(const QVector2D &force);

    void applyIdleEffect();

    QTimer m_windowPositionTimer;
    bool m_isMoving = false;
    QPointF m_lastWindowPosForTimer;
    int m_inactivityCounter = 0;
    QTimer m_resizeDebounceTimer;
    QSize m_lastSize;

    float m_absorptionScale = 1.0f;
    float m_absorptionOpacity = 1.0f;
    QPropertyAnimation* m_scaleAnimation = nullptr;
    QPropertyAnimation* m_opacityAnimation = nullptr;
    float m_absorptionPulse = 0.0f;
    QPropertyAnimation* m_pulseAnimation = nullptr;
    bool m_isClosingAfterAbsorption = false;

    bool m_isBeingAbsorbed = false;
    bool m_isAbsorbing = false;
    QString m_absorptionTargetId;

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

    double m_precalcMinDistance = 0.0;
    double m_precalcMaxDistance = 0.0;

    QThread m_physicsThread;
    QMutex m_dataMutex;
    std::atomic<bool> m_physicsRunning{true};
    std::vector<QPointF> m_safeControlPoints;

    void physicsThreadFunction();

    bool m_eventsEnabled = true;
    QTimer m_eventReEnableTimer;
    bool m_needsRedraw = false;
};

#endif // BLOBANIMATION_H