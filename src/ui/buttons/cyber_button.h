#ifndef CYBER_BUTTON_H
#define CYBER_BUTTON_H

#include <QPushButton>

/**
 * @brief A custom QPushButton styled with a cyberpunk aesthetic.
 *
 * This button features clipped corners, neon borders, and a subtle glow effect
 * that animates on hover and press. It supports two color schemes: primary (blue/cyan)
 * and secondary (pink/magenta), controlled by the `isPrimary` flag.
 * It also includes decorative corner markers and inner lines.
 */
class CyberButton final : public QPushButton {
    Q_OBJECT
    /** @brief Property controlling the intensity of the button's glow effect (0.0 to 1.0+). Animatable. */
    Q_PROPERTY(double glowIntensity READ GetGlowIntensity WRITE SetGlowIntensity)

public:
    /**
     * @brief Constructs a CyberButton.
     * Initializes the button style (transparent background, no border, font via stylesheet),
     * sets the cursor, and determines the color scheme based on the isPrimary flag.
     * @param text The text to display on the button.
     * @param parent Optional parent widget.
     * @param isPrimary If true, uses the primary (blue/cyan) color scheme; otherwise, uses the secondary (pink/magenta) scheme. Defaults to true.
     */
    explicit CyberButton(const QString &text, QWidget *parent = nullptr, bool isPrimary = true);

    /**
     * @brief Gets the current intensity of the glow effect.
     * @return The glow intensity value (typically between 0.0 and 1.0+).
     */
    double GetGlowIntensity() const { return glow_intensity_; }

    /**
     * @brief Sets the intensity of the glow effect.
     * Triggers a repaint of the button.
     * @param intensity The desired glow intensity.
     */
    void SetGlowIntensity(double intensity);

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom cyberpunk button appearance.
     * Renders the clipped background, neon border, glow effect, decorative lines/markers, and text
     * based on the primary/secondary color scheme and current state (enabled, down).
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Overridden enter event handler. Animates the glow intensity to a higher value.
     * @param event The enter event.
     */
    void enterEvent(QEvent *event) override;

    /**
     * @brief Overridden leave event handler. Animates the glow intensity back to its default value.
     * @param event The leave event.
     */
    void leaveEvent(QEvent *event) override;

    /**
     * @brief Overridden mouse press event handler. Sets the glow intensity to maximum and triggers a repaint.
     * @param event The mouse press event.
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief Overridden mouse release event handler. Sets the glow intensity back to the hover level and triggers a repaint.
     * @param event The mouse release event.
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    /** @brief Current intensity of the glow effect. Modified by animations and mouse events. */
    double glow_intensity_;
    /** @brief Flag determining the color scheme (true for primary blue/cyan, false for secondary pink/magenta). */
    bool is_primary_;
};

#endif //CYBER_BUTTON_H
