#ifndef DOTANIMATION_H
#define DOTANIMATION_H

#include <QWidget>
#include <QTimer>
#include <QPoint>
#include <QSize>
#include <QDateTime>
#include <vector>
#include "dot.h"

class DotAnimation : public QWidget {
    Q_OBJECT

public:
    explicit DotAnimation(QWidget *parent = nullptr);
    ~DotAnimation();

    void setColor(const QColor &color) {
        primaryColor = color;
        useDualColor = false;
    }

    void setColors(const QColor &primary, const QColor &secondary) {
        primaryColor = primary;
        secondaryColor = secondary;
        useDualColor = true;
    }

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

    private slots:
        void updateAnimation();
    void setupEventTracking();

private:
    void applyForceToAllDots(float forceX, float forceY);
    void initializeDots();
    void pauseIdleAnimation();
    void resumeIdleAnimation();

    QColor primaryColor = QColor(180, 180, 180);
    QColor secondaryColor = QColor(220, 50, 50);
    bool useDualColor = true;


    std::vector<Dot> dots;
    int numDots;
    float breathePhase;
    float springStiffness;
    float dampingFactor;
    QTimer *timer;
    QPoint lastWindowPos;
    QSize lastWindowSize;
    qint64 lastUpdateTime;
    bool isWindowTracking;
    float pulseAngleOffset{};

    bool idleAnimationActive;
    float idleAnimationPhase;
    QTimer *idleAnimationTimer;
    qint64 lastUserActionTime;
    bool strongPulseActive;
    int strongPulseTimer;
    float forceAngle;      
    float forceWidth;      
    int forceDirection;    
    float forceSpeed;
    float targetForceAngle;  
    bool moveToNewTarget;
    float lastTargetAngle;      

    float idleAnimationIntensity;


};

#endif 