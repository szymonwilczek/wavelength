#ifndef TAB_BUTTON_H
#define TAB_BUTTON_H

#include <QPushButton>

/**
 * @brief A custom QPushButton styled as a tab button with an animated underline effect.
 *
 * This button is designed for use in tab bars. It features a flat appearance,
 * specific cyberpunk-themed styling (colors, font) via stylesheet, and is checkable.
 * An underline effect is drawn below the button text. The underline is fully visible
 * when the button is active (`SetActive(true)` or checked) and animates smoothly
 * on mouse hover (enter/leave events) when inactive, controlled by the `underlineOffset` property.
 */
class TabButton final : public QPushButton {
    Q_OBJECT
    /**
     * @brief Property controlling the animation state of the underline effect on hover (0.0 to 5.0).
     * When the button is inactive, this property animates from 0.0 to 5.0 on enter, and back to 0.0 on leave.
     * The paintEvent uses this value (and its sine) to draw the partial, moving underline.
     */
    Q_PROPERTY(double underlineOffset READ UnderlineOffset WRITE SetUnderlineOffset)

public:
    /**
     * @brief Constructs a TabButton.
     * Initializes the button as flat, checkable, sets the cursor, and applies the
     * specific stylesheet for tab button appearance (colors, font, padding, etc.).
     * @param text The text to display on the tab button.
     * @param parent Optional parent widget.
     */
    explicit TabButton(const QString &text, QWidget *parent = nullptr);

    /**
     * @brief Gets the current value of the underline animation offset.
     * @return The underline offset value (typically 0.0 to 5.0).
     */
    double UnderlineOffset() const { return underline_offset_; }

    /**
     * @brief Sets the value of the underline animation offset.
     * Triggers a repaint of the button. Used by the QPropertyAnimation.
     * @param offset The desired underline offset value.
     */
    void SetUnderlineOffset(double offset);

    /**
     * @brief Sets the active state of the tab button.
     * An active button displays a full, static underline. An inactive button
     * shows the animated underline on hover. This also affects the underline color.
     * Triggers a repaint.
     * @param active True to set the button as active, false otherwise.
     */
    void SetActive(bool active);

protected:
    /**
     * @brief Overridden enter event handler. Starts the animation to show the underline if the button is inactive.
     * @param event The enter event.
     */
    void enterEvent(QEvent *event) override;

    /**
     * @brief Overridden leave event handler. Starts the animation to hide the underline if the button is inactive.
     * @param event The leave event.
     */
    void leaveEvent(QEvent *event) override;

    /**
     * @brief Overridden paint event handler. Draws the custom underline effect.
     * Calls the base class paintEvent first. Then, draws either a full static underline
     * (if active) or an animated, partial underline (if inactive and underline_offset_ > 0)
     * below the button text. The underline color also depends on the active state.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /** @brief Current animation offset for the underline effect (0.0 to 5.0). */
    double underline_offset_;
    /** @brief Flag indicating if the tab button is currently considered active (displays full underline). */
    bool is_active_;
};


#endif //TAB_BUTTON_H
