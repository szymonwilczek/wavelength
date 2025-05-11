#ifndef USER_INFO_LABEL_H
#define USER_INFO_LABEL_H

#include <QLabel>

/**
 * @brief A custom QLabel that displays text alongside a colored circle with a glow effect.
 *
 * This label is designed to show user information (like a username) preceded by a
 * visual indicator (a colored circle). The circle's color can be set using SetShapeColor,
 * and it features a multi-layered glow effect rendered in the paintEvent. The size hint
 * is adjusted to accommodate the circle and its glow.
 */
class UserInfoLabel final : public QLabel {
    Q_OBJECT

public:
    /**
     * @brief Constructs a UserInfoLabel.
     * Initializes default parameters for the circle and glow effect (diameter, pen width,
     * glow layers, spread) and calculates the total space required for the shape.
     * @param parent Optional parent widget.
     */
    explicit UserInfoLabel(QWidget *parent = nullptr);

    /**
     * @brief Sets the color of the circle and its glow.
     * Triggers a repaint of the label.
     * @param color The desired QColor for the shape.
     */
    void SetShapeColor(const QColor &color);

    /**
     * @brief Returns the recommended size for the label.
     * Calculates the size based on the text size plus the space required for the
     * colored circle, its glow, and padding.
     * @return The calculated QSize hint.
     */
    QSize sizeHint() const override;

    /**
     * @brief Returns the minimum recommended size for the label.
     * Similar to sizeHint, but based on the minimum text size.
     * @return The calculated minimum QSize hint.
     */
    QSize minimumSizeHint() const override;

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom label appearance.
     * Renders the background, the colored circle with its multi-layered glow effect,
     * and finally the label text positioned to the right of the shape.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /** @brief The base color used for the circle and its glow. */
    QColor shape_color_;
    /** @brief Diameter of the main colored circle indicator. */
    int circle_diameter_;
    /** @brief The thickness of the pen used to draw the main circle. */
    qreal pen_width_;
    /** @brief Number of layers used to render the glow effect. */
    int glow_layers_;
    /** @brief How much each glow layer expands outwards relatively to the previous one. */
    qreal glow_spread_;
    /** @brief Horizontal padding between the right edge of the glow and the start of the text. */
    int shape_padding_;
    /** @brief Pre-calculated total width required for the circle and its maximum glow extent. */
    int total_shape_size_;
};

#endif // USER_INFO_LABEL_H
