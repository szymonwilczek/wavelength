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

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;

    private slots:
        void updateAnimation();

private:

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