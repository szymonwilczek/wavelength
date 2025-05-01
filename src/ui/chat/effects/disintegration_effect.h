#ifndef DISINTEGRATION_EFFECT_H
#define DISINTEGRATION_EFFECT_H

#include <QGraphicsEffect>
#include <QPainter>
#include <QRandomGenerator>
#include <QCache>
#include <QVector>
#include <QPoint>
#include <QPixmap>
#include <QObject>
#include <QtGlobal>

/**
 * @brief A QGraphicsEffect that simulates a disintegration or dissolving effect.
 *
 * This effect takes the source pixmap of the widget it's applied to and redraws it
 * as a collection of scattered particles. The scattering increases and opacity decreases
 * based on the `progress` property (0.0 = no effect, 1.0 = fully disintegrated).
 * It uses precomputed random offsets and caching to optimize performance.
 */
class DisintegrationEffect final : public QGraphicsEffect {
    Q_OBJECT
    /** @brief Property controlling the progress of the disintegration effect (0.0 to 1.0). Animatable. */
    Q_PROPERTY(qreal progress READ GetProgress WRITE SetProgress)

public:
    /**
     * @brief Constructs a DisintegrationEffect.
     * Initializes the effect and precomputes a set of random offsets used for particle scattering.
     * @param parent Optional parent QObject.
     */
    explicit DisintegrationEffect(QObject* parent = nullptr);

    /**
     * @brief Sets the progress of the disintegration effect.
     * Clamps the value between 0.0 and 1.0 and triggers an update of the effect.
     * @param progress The desired progress level (0.0 to 1.0).
     */
    void SetProgress(qreal progress);

    /**
     * @brief Gets the current progress of the disintegration effect.
     * @return The current progress level (0.0 to 1.0).
     */
    qreal GetProgress() const { return progress_; }

protected:
    /**
     * @brief Overridden draw method that renders the effect.
     * If progress is minimal, draws the source directly. Otherwise, retrieves the source pixmap,
     * checks a cache for a pre-rendered result at the current progress level. If not cached,
     * it generates the effect by drawing parts of the source pixmap as scattered particles
     * with decreasing opacity and increasing spread based on `progress_`. The result is
     * stored in the cache. Finally, the cached (or newly generated) pixmap is drawn.
     * @param painter The QPainter to use for drawing.
     */
    void draw(QPainter* painter) override;

private:
    /** @brief Current progress level of the disintegration (0.0 to 1.0). */
    qreal progress_;
    /** @brief Cache storing pre-rendered effect pixmaps keyed by progress level string for performance. */
    QCache<QString, QPixmap> particle_cache_;
    /** @brief Vector storing precomputed random offsets to avoid runtime random generation during drawing. */
    QVector<QPoint> precomputed_offsets_;
};

#endif //DISINTEGRATION_EFFECT_H