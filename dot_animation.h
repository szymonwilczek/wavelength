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

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

    private slots:
        void updateAnimation();
    void setupEventTracking();

private:
    void applyForceToAllDots(float forceX, float forceY);
    void initializeDots();

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
};

#endif // DOTANIMATION_H