#ifndef CYBERPUNK_BUTTON_H
#define CYBERPUNK_BUTTON_H

#include <QPushButton>

class QGraphicsDropShadowEffect;
class QPropertyAnimation;

/**
 * @brief A custom QPushButton styled with a cyberpunk aesthetic, featuring animated glow effects.
 *
 * This button provides a distinct visual style with a neon blue frame and text.
 * It includes a glow effect for both the frame and the text, which animates
 * smoothly on mouse hover (enter/leave events) using QPropertyAnimation.
 * The intensity of the glow and the thickness of the frame adjust based on the animation state.
 */
class CyberpunkButton final : public QPushButton {
    Q_OBJECT

    /**
     * @brief Property controlling the intensity of the button's glow effect (0.0 to 1.0). Animatable.
     * It affects the brightness and thickness of the frame border and the text's drop shadow.
     */
    Q_PROPERTY(qreal glowIntensity READ GetGlowIntensity WRITE SetGlowIntensity) // Corrected getter name

public:
    /**
     * @brief Constructs a CyberpunkButton.
     * Initializes the button's appearance (stylesheet for text, colors, size policy),
     * sets up the text glow effect (QGraphicsDropShadowEffect), and configures the
     * QPropertyAnimation for the hover glow effect.
     * @param text The text to display on the button.
     * @param parent Optional parent widget.
     */
    explicit CyberpunkButton(const QString &text, QWidget *parent = nullptr);

    /**
     * @brief Destructor. Cleans up allocated animation objects.
     */
    ~CyberpunkButton() override;

    /**
     * @brief Gets the current intensity of the glow effect.
     * @return The glow intensity value (typically between 0.0 and 1.0).
     */
    qreal GetGlowIntensity() const { return glow_intensity_; }

    /**
     * @brief Sets the intensity of the glow effect and updates the button's appearance.
     * Modifies the text glow effect's blur radius and color alpha based on the intensity.
     * Triggers a repaint of the button.
     * @param intensity The desired glow intensity (typically between 0.0 and 1.0).
     */
    void SetGlowIntensity(qreal intensity);

protected:
    /**
     * @brief Overridden enter event handler. Starts the animation to increase glow intensity.
     * @param event The enter event.
     */
    void enterEvent(QEvent *event) override;

    /**
     * @brief Overridden leave event handler. Starts the animation to decrease glow intensity.
     * @param event The leave event.
     */
    void leaveEvent(QEvent *event) override;

    /**
     * @brief Overridden resize event handler. Currently only calls the base class implementation.
     * @param event The resize event.
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief Overridden paint event handler. Draws the custom cyberpunk button frame.
     * Calls the base class paintEvent first, then draws a rounded rectangle frame
     * whose color alpha and thickness depend on the current glow_intensity_.
     * Optionally draws a secondary outer glow for the frame at higher intensities.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /** @brief Animation controlling the glowIntensity property on hover. */
    QPropertyAnimation *glow_animation_;
    /** @brief Graphics effect providing the glow/shadow for the button text. */
    QGraphicsDropShadowEffect *glow_effect_;

    /** @brief Current intensity of the glow effect (0.0 to 1.0). Controlled by glow_animation_. */
    qreal glow_intensity_;
    /** @brief Base color for the neon frame (alpha adjusted by glow_intensity_). */
    QColor frame_color_;
    /** @brief Base color for the frame's outer glow (alpha adjusted by glow_intensity_). */
    QColor glow_color_;
    /** @brief Base width for the frame border (thickness adjusted by glow_intensity_). */
    int frame_width_;
};

#endif // CYBERPUNK_BUTTON_H
