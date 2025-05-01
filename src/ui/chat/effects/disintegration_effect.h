#ifndef DISINTEGRATION_EFFECT_H
#define DISINTEGRATION_EFFECT_H
#include <QGraphicsEffect>
#include <QPainter>
#include <QRandomGenerator>
#include <QCache>

class DisintegrationEffect final : public QGraphicsEffect {
public:
    explicit DisintegrationEffect(QObject* parent = nullptr);

    void SetProgress(qreal progress);

    qreal GetProgress() const { return progress_; }

protected:
    void draw(QPainter* painter) override;

private:
    qreal progress_;
    QCache<QString, QPixmap> particle_cache_;
    QVector<QPoint> precomputed_offsets_;
};

#endif //DISINTEGRATION_EFFECT_H