#ifndef CYBER_CHECKBOX_H
#define CYBER_CHECKBOX_H

#include <QCheckBox>

/**
 * @brief A custom QCheckBox styled with a cyberpunk aesthetic.
 *
 * This checkbox features a custom-drawn square indicator with clipped corners,
 * a neon blue border, and a subtle glow effect that animates on hover.
 * The checkmark is rendered as a stylized 'X'. The text uses a specific
 * cyberpunk-themed font and color.
 */
class CyberCheckBox final : public QCheckBox {
    Q_OBJECT
    /** @brief Property controlling the intensity of the checkbox border glow (0.0 to 1.0+). Animatable. */
    Q_PROPERTY(double glowIntensity READ GetGlowIntensity WRITE SetGlowIntensity)

public:
    /**
     * @brief Constructs a CyberCheckBox.
     * Initializes the checkbox with text, applies basic styling (spacing, background, color, font)
     * via stylesheet, and sets a minimum height.
     * @param text The label text for the checkbox.
     * @param parent Optional parent widget.
     */
    explicit CyberCheckBox(const QString &text, QWidget *parent = nullptr);

    /**
     * @brief Returns the recommended size for the checkbox.
     * Ensures a minimum height is respected.
     * @return The calculated QSize hint.
     */
    QSize sizeHint() const override;

    /**
     * @brief Gets the current intensity of the glow effect.
     * @return The glow intensity value (typically 0.5 to 0.9).
     */
    double GetGlowIntensity() const { return glow_intensity_; }

    /**
     * @brief Sets the intensity of the glow effect.
     * Triggers a repaint of the checkbox.
     * @param intensity The desired glow intensity.
     */
    void SetGlowIntensity(double intensity);

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom checkbox appearance.
     * Renders the clipped square indicator (background, border, glow), the 'X' checkmark
     * if checked, and the label text using the specified cyberpunk style.
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

private:
    /** @brief Current intensity of the glow effect. Modified by animations on hover. */
    double glow_intensity_;
};

#endif //CYBER_CHECKBOX_H
