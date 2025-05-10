#ifndef CYBER_INPUT_H
#define CYBER_INPUT_H

#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

/**
 * @brief A custom QLineEdit styled with a cyberpunk aesthetic.
 *
 * This line edit features a dark background, clipped corners, a neon blue border,
 * and a subtle glow effect that animates on focus and hover. It also implements
 * a custom-drawn blinking cursor instead of the default system cursor.
 */
class CyberLineEdit final : public QLineEdit {
    Q_OBJECT
    /** @brief Property controlling the intensity of the border glow effect (0.0 to 1.0). Animatable. */
    Q_PROPERTY(double glowIntensity READ GetGlowIntensity WRITE SetGlowIntensity)

public:
    /**
     * @brief Constructs a CyberLineEdit.
     * Initializes the widget with cyberpunk styling (border, background, padding, font)
     * via stylesheet, sets the text color, cursor type, minimum height, and starts a
     * timer for the custom cursor blinking animation.
     * @param parent Optional parent widget.
     */
    explicit CyberLineEdit(QWidget *parent = nullptr);

    /**
     * @brief Destructor. Stops the cursor blink timer.
     */
    ~CyberLineEdit() override;

    /**
     * @brief Returns the recommended size for the line edit.
     * Enforces a minimum height of 30 pixels.
     * @return The calculated QSize hint.
     */
    QSize sizeHint() const override;

    /**
     * @brief Gets the current intensity of the glow effect.
     * @return The glow intensity value (typically 0.0 to 1.0).
     */
    double GetGlowIntensity() const { return glow_intensity_; }

    /**
     * @brief Sets the intensity of the glow effect.
     * Triggers a repaint of the line edit.
     * @param intensity The desired glow intensity.
     */
    void SetGlowIntensity(double intensity);

    /**
     * @brief Calculates the rectangle occupied by the custom cursor.
     * Determines the cursor's position based on the text content and cursor position,
     * and returns a QRect representing its visual bounds.
     * @return The QRect for the custom cursor.
     */
    QRect CyberCursorRect() const;

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom line edit appearance.
     * Renders the clipped background, border, glow effect, placeholder text (if applicable),
     * the actual text content (handling password mode), and the custom blinking cursor.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Overridden focus in event handler. Starts the cursor blink timer and animates the glow effect in.
     * @param event The focus event.
     */
    void focusInEvent(QFocusEvent *event) override;

    /**
     * @brief Overridden focus out event handler. Stops the cursor blink timer and animates the glow effect out.
     * @param event The focus event.
     */
    void focusOutEvent(QFocusEvent *event) override;

    /**
     * @brief Overridden enter event handler. Animates the glow effect partially in if the widget doesn't have focus.
     * @param event The enter event.
     */
    void enterEvent(QEvent *event) override;

    /**
     * @brief Overridden leave event handler. Animates the glow effect out if the widget doesn't have focus.
     * @param event The leave event.
     */
    void leaveEvent(QEvent *event) override;

    /**
     * @brief Overridden key press event handler. Resets the cursor blink state and timer on key press.
     * @param event The key event.
     */
    void keyPressEvent(QKeyEvent *event) override;

private:
    /** @brief Current intensity of the glow effect. Modified by animations on focus/hover. */
    double glow_intensity_;
    /** @brief Timer controlling the blinking animation of the custom cursor. */
    QTimer *cursor_blink_timer_;
    /** @brief Flag indicating whether the custom cursor is currently visible (part of the blink cycle). */
    bool cursor_visible_;
};


#endif //CYBER_INPUT_H
