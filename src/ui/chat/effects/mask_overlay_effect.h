#ifndef MASK_OVERLAY_H
#define MASK_OVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPaintEvent>

/**
 * @brief A custom overlay widget that creates a "reveal" effect using a moving scanline.
 *
 * This widget sits on top of another widget and initially covers it with a semi-transparent
 * mask featuring glitch-like patterns. A bright scanline moves down the widget. As the scanline
 * progresses (controlled by SetRevealProgress), the masked area above the scanline becomes
 * transparent, revealing the content underneath. The widget is transparent to mouse events,
 * allowing interaction with the underlying widget.
 */
class MaskOverlayEffect final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a MaskOverlay widget.
     * Initializes the widget attributes (transparent for mouse events, no system background),
     * sets up and starts the timer for the scanline animation.
     * @param parent Optional parent widget.
     */
    explicit MaskOverlayEffect(QWidget *parent = nullptr);

public slots:
    /**
     * @brief Sets the vertical reveal progress as a percentage.
     * Controls how much of the underlying widget is revealed (from top to bottom).
     * @param percentage The reveal percentage (0-100). Values outside this range are clamped.
     */
    void SetRevealProgress(int percentage);

    /**
     * @brief Starts or restarts the scanline animation and reveal effect.
     * Resets the reveal percentage and scanline position, makes the overlay visible,
     * and ensures the scanline timer is running.
     */
    void StartScanning();

    /**
     * @brief Stops the scanline animation and hides the overlay.
     */
    void StopScanning();

protected:
    /**
     * @brief Overridden paint event handler. Draws the overlay's appearance.
     * Renders the semi-transparent mask over the unrevealed area (with glitch patterns),
     * draws the moving scanline with a glow effect, and potentially adds effects
     * near the reveal boundary.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private slots:
    /**
     * @brief Slot called by the scan_timer_ to update the vertical position of the scanline.
     * Moves the scanline downwards and wraps it back to the top when it reaches the bottom.
     * Triggers a repaint.
     */
    void UpdateScanLine();

private:
    /** @brief The current reveal progress percentage (0-100). Determines the height of the revealed area. */
    int reveal_percentage_;
    /** @brief The current vertical position (y-coordinate) of the scanline. */
    int scanline_y_;
    /** @brief Timer controlling the animation of the scanline's movement. */
    QTimer *scan_timer_;
};

#endif // MASK_OVERLAY_H
