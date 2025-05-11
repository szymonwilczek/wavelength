#ifndef CLICKABLE_COLOR_PREVIEW_H
#define CLICKABLE_COLOR_PREVIEW_H

#include <QWidget>

/**
 * @brief A small, clickable widget that displays a solid color preview.
 *
 * This widget is typically used in settings panels to show a color and allow the user
 * to click it, usually to open a color picker dialog. It displays the color set by
 * SetColor() as its background and draws a thin border around itself. It emits a
 * clicked() signal when pressed with the left mouse button.
 */
class ClickableColorPreview final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a ClickableColorPreview widget.
     * Sets a fixed size, enables the pointing hand cursor, and configures background drawing.
     * @param parent Optional parent widget.
     */
    explicit ClickableColorPreview(QWidget *parent = nullptr);

public slots:
    /**
     * @brief Sets the background color to be displayed by the preview widget.
     * Updates the widget's palette and triggers a repaint. If the color is invalid,
     * a transparent background is set.
     * @param color The QColor to display.
     */
    void SetColor(const QColor &color);

signals:
    /**
     * @brief Emitted when the widget is clicked with the left mouse button.
     */
    void clicked();

protected:
    /**
     * @brief Overridden mouse press event handler.
     * Emits the clicked() signal if the left mouse button is pressed.
     * @param event The mouse event.
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief Overridden paint event handler. Draws the widget's appearance.
     * Fills the background with the color set in the palette (via SetColor) and
     * draws a thin border around the widget.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;
};

#endif //CLICKABLE_COLOR_PREVIEW_H
