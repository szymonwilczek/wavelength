#ifndef ELECTRONIC_SHUTDOWN_EFFECT_H
#define ELECTRONIC_SHUTDOWN_EFFECT_H

#include <QGraphicsEffect>
#include <QPainter>
#include <QRandomGenerator>
#include <QDateTime>
#include <QCache>

class ElectronicShutdownEffect : public QGraphicsEffect {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
    
public:
    ElectronicShutdownEffect(QObject* parent = nullptr);

    void setProgress(qreal progress);

    qreal progress() const { 
        return m_progress; 
    }

protected:
    void draw(QPainter* painter) override;

private:
    qreal m_progress;
    qreal m_lastProgress;
    QCache<QString, QPixmap> m_resultCache;
    quint32 m_randomSeed;
};

#endif // ELECTRONIC_SHUTDOWN_EFFECT_H