#ifndef OVERLAY_WIDGET_H
#define OVERLAY_WIDGET_H

#include <QWidget>
#include <QPainter>
#include <QResizeEvent>
#include <QRect>

/**
 * @brief A semi-transparent overlay widget used to dim the background content.
 *
 * This widget is typically placed over another widget (its parent) to create a dimming effect,
 * often used when a modal dialog or sidebar is shown. It can exclude a specific rectangular area
 * (like a navigation bar) from being dimmed. The opacity of the overlay can be animated.
 * It automatically resizes to match its parent widget.
 */
class OverlayWidget final : public QWidget {
    Q_OBJECT
    /** @brief Property controlling the opacity of the overlay (0.0 = transparent, 1.0 = fully opaque black). Animatable. */
    Q_PROPERTY(qreal opacity READ GetOpacity WRITE SetOpacity)

public:
    /**
     * @brief Constructs an OverlayWidget.
     * Sets necessary widget attributes for transparency and event handling. Installs an event filter
     * on the parent to track resize events. Attempts to find a Navbar child in the parent to exclude its area.
     * @param parent The parent widget over which the overlay will be displayed.
     */
    explicit OverlayWidget(QWidget *parent = nullptr);

    /**
     * @brief Updates the geometry of the overlay widget.
     * Typically called when the parent widget resizes.
     * @param rect The new geometry rectangle, usually matching the parent's bounds.
     */
    void UpdateGeometry(const QRect &rect);

    /**
     * @brief Gets the current opacity of the overlay.
     * @return The current opacity value (0.0 to 1.0).
     */
    qreal GetOpacity() const { return opacity_; }

    /**
     * @brief Sets the opacity of the overlay.
     * Triggers a repaint of the widget.
     * @param opacity The desired opacity (0.0 to 1.0).
     */
    void SetOpacity(qreal opacity);

private:
    /** @brief Current opacity level of the overlay (0.0 to 1.0). */
    qreal opacity_;
    /** @brief Rectangle defining an area (e.g., Navbar) to be excluded from the dimming effect. */
    QRect exclude_rect_;

protected:
    /**
     * @brief Filters events from the parent widget.
     * Specifically, watches for Resize events on the parent to call UpdateGeometry().
     * @param watched The object that generated the event.
     * @param event The event being processed.
     * @return True if the event was handled, false otherwise.
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

    /**
     * @brief Overridden paint event handler. Draws the semi-transparent overlay.
     * Fills the widget's area with a black color whose alpha is determined by opacity_.
     * Excludes the exclude_rect_ area from being painted using clipping.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;
};


#endif //OVERLAY_WIDGET_H
