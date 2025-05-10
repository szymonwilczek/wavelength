#ifndef ELECTRONIC_SHUTDOWN_EFFECT_H
#define ELECTRONIC_SHUTDOWN_EFFECT_H

#include <QGraphicsEffect>
#include <QPainter>
#include <QRandomGenerator>
#include <QDateTime>
#include <QCache>
#include <QPixmap>
#include <QObject>
#include <QtGlobal>

/**
 * @brief A QGraphicsEffect that simulates an old CRT monitor or electronic device shutting down.
 *
 * This effect animates in two phases based on the `progress` property (0.0 to 1.0):
 * 1. (0.0-0.7): The source image vertically collapses into a thin horizontal line,
 *    accompanied by visual glitches (random line shifts) and scanline effects.
 * 2. (0.7-1.0): The horizontal line fades out, shrinks horizontally, increases in brightness,
 *    and exhibits flickering and glow effects before disappearing completely.
 *
 * The effect uses caching (QCache) to store intermediate results for performance.
 */
class ElectronicShutdownEffect final : public QGraphicsEffect {
    Q_OBJECT
    /** @brief Property controlling the progress of the shutdown animation (0.0 to 1.0). Animatable. */
    Q_PROPERTY(qreal progress READ GetProgress WRITE SetProgress)

public:
    /**
     * @brief Constructs an ElectronicShutdownEffect.
     * Initializes the effect, sets up the result cache, and seeds the random generator.
     * @param parent Optional parent QObject.
     */
    explicit ElectronicShutdownEffect(QObject *parent = nullptr);

    /**
     * @brief Sets the progress of the shutdown animation.
     * Clamps the value between 0.0 and 1.0 and triggers an update of the effect.
     * @param progress The desired progress level (0.0 to 1.0).
     */
    void SetProgress(qreal progress);

    /**
     * @brief Gets the current progress of the shutdown animation.
     * @return The current progress level (0.0 to 1.0).
     */
    qreal GetProgress() const {
        return progress_;
    }

protected:
    /**
     * @brief Overridden draw method that renders the shutdown effect.
     * Retrieves the source pixmap. If progress is minimal, draws the source directly.
     * If progress is maximal, draws nothing. Otherwise, checks a cache for a pre-rendered
     * result at a similar progress level. If not cached or progress changed significantly,
     * it generates the effect based on the current phase (collapsing or fading line),
     * applying glitches, scanlines, brightness changes, and glow effects. The result is
     * stored in the cache. Finally, the cached (or newly generated) pixmap is drawn.
     * @param painter The QPainter to use for drawing.
     */
    void draw(QPainter *painter) override;

private:
    /** @brief Current progress level of the shutdown animation (0.0 to 1.0). */
    qreal progress_;
    /** @brief Stores the progress value from the last time the cache was updated, used to optimize cache usage. */
    qreal last_progress_;
    /** @brief Cache storing pre-rendered effect pixmaps keyed by progress level string for performance. */
    QCache<QString, QPixmap> result_cache_;
};

#endif // ELECTRONIC_SHUTDOWN_EFFECT_H
