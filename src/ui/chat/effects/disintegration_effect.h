#ifndef DISINTEGRATION_EFFECT_H
#define DISINTEGRATION_EFFECT_H
#include <QGraphicsEffect>
#include <QPainter>
#include <QRandomGenerator>
#include <QCache>

class DisintegrationEffect : public QGraphicsEffect {
public:
    DisintegrationEffect(QObject* parent = nullptr);

    void setProgress(qreal progress);

    qreal progress() const { return m_progress; }

protected:
    void draw(QPainter* painter) override;

private:
    qreal m_progress;
    QCache<QString, QPixmap> m_particleCache;
    QVector<QPoint> m_precomputedOffsets;
};

#endif //DISINTEGRATION_EFFECT_H