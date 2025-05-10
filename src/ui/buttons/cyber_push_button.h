#ifndef CYBER_PUSH_BUTTON_H
#define CYBER_PUSH_BUTTON_H

#include <QPushButton>
#include <QTimer>

/**
 * @brief A custom QPushButton styled with a cyberpunk aesthetic, featuring pulsing glow effects.
 *
 * This button has a dark blue background, clipped corners, a neon blue border,
 * and neon blue text. It includes decorative corner markers and inner-dotted lines.
 * A key feature is a subtle pulsing glow effect managed by a QTimer, which intensifies
 * on hover and press events.
 */
class CyberPushButton final : public QPushButton {
    Q_OBJECT
    /** @brief Property controlling the intensity of the button's glow effect (0.0 to 1.0+). Animatable. */
    Q_PROPERTY(double glowIntensity READ GetGlowIntensity WRITE SetGlowIntensity) // Corrected property name accessors

public:
    /**
     * @brief Constructs a CyberPushButton.
     * Initializes the button style (transparent background, no border via stylesheet),
     * sets the cursor, starts a timer for the pulsing glow effect, and sets the initial base glow.
     * @param text The text to display on the button.
     * @param parent Optional parent widget.
     */
    explicit CyberPushButton(const QString &text, QWidget *parent = nullptr);

    /**
     * @brief Destructor. Stops the pulse timer.
     */
    ~CyberPushButton() override {
        if (pulse_timer_) {
            pulse_timer_->stop();
        }
    }

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
    void SetGlowIntensity(const double intensity) {
        glow_intensity_ = intensity;
        update();
    }

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom cyberpunk button appearance.
     * Renders the clipped background, neon border, glow effect, decorative lines/markers, and text.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Overridden enter event handler. Increases the base glow intensity when the mouse enters.
     * @param event The enter event.
     */
    void enterEvent(QEvent *event) override {
        base_glow_intensity_ = 0.8;
        QPushButton::enterEvent(event);
    }

    /**
     * @brief Overridden leave event handler. Resets the base glow intensity when the mouse leaves.
     * @param event The leave event.
     */
    void leaveEvent(QEvent *event) override {
        base_glow_intensity_ = 0.5;
        QPushButton::leaveEvent(event);
    }

    /**
     * @brief Overridden mouse press event handler. Maximizes the base glow intensity when pressed.
     * @param event The mouse press event.
     */
    void mousePressEvent(QMouseEvent *event) override {
        base_glow_intensity_ = 1.0;
        QPushButton::mousePressEvent(event);
    }

    /**
     * @brief Overridden mouse release event handler. Sets the base glow intensity back to the hover level if still hovering.
     * @param event The mouse release event.
     */
    void mouseReleaseEvent(QMouseEvent *event) override {
        base_glow_intensity_ = 0.8;
        QPushButton::mouseReleaseEvent(event);
    }

private:
    /** @brief Current overall glow intensity, influenced by base_glow_intensity_ and pulse_timer_. */
    double glow_intensity_;
    /** @brief Base level for the glow effect, modified by hover/press states. */
    double base_glow_intensity_;
    /** @brief Timer responsible for the subtle pulsing animation of the glow effect. */
    QTimer *pulse_timer_;
};

#endif //CYBER_PUSH_BUTTON_H
