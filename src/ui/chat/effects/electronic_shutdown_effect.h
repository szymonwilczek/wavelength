#ifndef ELECTRONIC_SHUTDOWN_EFFECT_H
#define ELECTRONIC_SHUTDOWN_EFFECT_H

#include <QGraphicsEffect>
#include <QPainter>
#include <QRandomGenerator>
#include <QDateTime>
#include <QCache>

class ElectronicShutdownEffect final : public QGraphicsEffect {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
    
public:
    explicit ElectronicShutdownEffect(QObject* parent = nullptr);

    void SetProgress(qreal progress);

    qreal GetProgress() const {
        return progress_;
    }

protected:
    void draw(QPainter* painter) override;

private:
    qreal progress_;
    qreal last_progress_;
    QCache<QString, QPixmap> result_cache_;
    quint32 random_seed_;
};

#endif // ELECTRONIC_SHUTDOWN_EFFECT_H