#ifndef BLOBANIMATION_H
#define BLOBANIMATION_H

#include <deque>
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
#include "blob_absorption.h"
#include "blob_event_handler.h"
#include "blob_transition_manager.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>

class BlobAnimation : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit BlobAnimation(QWidget *parent = nullptr);

    void handleResizeTimeout();

    void checkWindowPosition();

    ~BlobAnimation() override;

    void setBackgroundColor(const QColor &color);

    void setGridColor(const QColor &color);

    void setGridSpacing(int spacing);

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

    void setLifeColor(const QColor& color);
    void resetLifeColor();

    void startBeingAbsorbed();
    void finishBeingAbsorbed();
    void cancelAbsorption();
    void startAbsorbing(const QString& targetId);
    void finishAbsorbing(const QString& targetId);
    void cancelAbsorbing(const QString& targetId);
    void updateAbsorptionProgress(float progress);

    bool isBeingAbsorbed() const { return m_absorption.isBeingAbsorbed(); }
    bool isAbsorbing() const { return m_absorption.isAbsorbing(); }


protected:
    void paintEvent(QPaintEvent *event) override;

    QRectF calculateBlobBoundingRect();

    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void updateBlobGeometry();

    void drawGrid();

private slots:
    void updateAnimation();

    void processMovementBuffer();

    void updatePhysics();


    void onStateResetTimeout();

private:
    void initializeBlob();

    void switchToState(BlobConfig::AnimationState newState);

    void applyForces(const QVector2D &force);

    void applyIdleEffect();

    BlobAbsorption m_absorption;
    BlobEventHandler m_eventHandler;
    BlobTransitionManager m_transitionManager;

    QTimer m_windowPositionTimer;
    QPointF m_lastWindowPosForTimer;
    QTimer m_resizeDebounceTimer;
    QSize m_lastSize;
    QColor m_defaultLifeColor;
    bool m_originalBorderColorSet = false;

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


    double m_precalcMinDistance = 0.0;
    double m_precalcMaxDistance = 0.0;

    QMutex m_dataMutex;
    std::atomic<bool> m_physicsRunning{true};
    std::vector<QPointF> m_safeControlPoints;

    bool m_eventsEnabled = true;
    QTimer m_eventReEnableTimer;
    bool m_needsRedraw = false;

    std::thread m_physicsThread;
    std::atomic<bool> m_physicsActive{true};
    std::mutex m_pointsMutex;
    std::condition_variable m_physicsCondition;
    bool m_needsUpdate{false};

    void physicsThreadFunction();

    QOpenGLShaderProgram* m_shaderProgram = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;

    // Bufor dla geometrii bloba
    std::vector<GLfloat> m_glVertices;
};

#endif // BLOBANIMATION_H