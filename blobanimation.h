#ifndef BLOBANIMATION_H
#define BLOBANIMATION_H

#include <QWidget>
#include <QTimer>
#include <QVector2D>
#include <QElapsedTimer>
#include <QPainterPath>
#include <QVector>
#include <QPointF>

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

    private slots:
        void updateAnimation();

private:

    enum AnimationState {
        IDLE,
        MOVING,
        RESIZING
    };

    AnimationState m_currentState = IDLE;

    // efekt Idle
    QTimer m_idleTimer;
    double m_idleWavePhase = 0.0;
    double m_idleWaveFrequency = 2.0;
    double m_idleWaveAmplitude = 1.0;

    void applyIdleEffect();
    void switchToState(AnimationState newState);

    QTimer m_stateResetTimer;

    QColor m_backgroundColor;
    QColor m_gridColor;
    int m_gridSpacing{};

    QVector<QPointF> m_controlPoints;
    QVector<QPointF> m_targetPoints;
    QVector<QPointF> m_velocity;
    QPointF m_blobCenter;
    float m_blobRadius;


    QVector2D m_lastWindowVelocity;
    QPointF m_lastWindowPos;
    QElapsedTimer m_physicsTimer;


    QTimer m_animationTimer;
    int m_numPoints;


    QColor m_borderColor;
    int m_glowRadius;
    int m_borderWidth;


    void initializeBlob();
    void applyForces(const QVector2D &force);
    QPainterPath createBlobPath() const;
    void updateBlobPhysics();
    void smoothBlobShape();
    void handleBorderCollisions();
    void constrainNeighborDistances();
    bool isValidPoint(const QPointF &point) const;
    void validateAndRepairControlPoints();
    void stabilizeBlob();
};

#endif