#ifndef CYBER_LONG_TEXT_DISPLAY_H
#define CYBER_LONG_TEXT_DISPLAY_H

#include <QPainter>
#include <QDateTime>
#include <QFontMetrics>
#include <QDebug>
#include <QScrollBar>
#include <QTimer>
#include <QPaintEvent>

/**
 * @brief A custom widget for displaying potentially long text with cyberpunk aesthetics and efficient rendering.
 *
 * This widget takes a long string, processes it by wrapping lines based on the widget's width,
 * and renders only the visible portion of the text. It includes a cyberpunk-style background
 * gradient and scanline effect. Text processing is delayed and cached to avoid excessive
 * recalculations during resizing. It supports vertical scrolling via the SetScrollPosition method
 * and emits a signal indicating the total required height for the content.
 */
class CyberLongTextDisplay final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a CyberLongTextDisplay widget.
     * Initializes the widget with default size policies, font, text color, and sets up
     * a timer for delayed text processing.
     * @param text The initial text content to display.
     * @param text_color The color for the displayed text.
     * @param parent Optional parent widget.
     */
    explicit CyberLongTextDisplay(const QString& text, const QColor& text_color, QWidget* parent = nullptr);

    /**
     * @brief Sets the text content to be displayed.
     * Updates the internal original text, invalidates the processed text cache,
     * triggers delayed processing, and schedules a repaint.
     * @param text The new text content.
     */
    void SetText(const QString& text);

    /**
     * @brief Sets the color for the displayed text.
     * Updates the internal text color and schedules a repaint.
     * @param color The new text color.
     */
    void SetTextColor(const QColor& color);

    /**
     * @brief Returns the recommended size for the widget based on the processed text content.
     * The height is calculated based on the number of processed lines and font metrics.
     * @return The calculated QSize hint.
     */
    QSize sizeHint() const override;

    /**
     * @brief Sets the vertical scroll position.
     * This determines which part of the processed text is rendered in the paintEvent.
     * Schedules a repaint if the position changes.
     * @param position The vertical scroll position in pixels.
     */
    void SetScrollPosition(int position);

protected:
    /**
     * @brief Overridden paint event handler. Draws the widget's appearance.
     * Ensures text is processed, draws the background gradient, renders the visible lines
     * of the processed text based on the scroll position, and applies a scanline effect.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Overridden resize event handler.
     * Invalidates the processed text cache and triggers delayed text processing
     * when the widget's size changes.
     * @param event The resize event.
     */
    void resizeEvent(QResizeEvent* event) override;

private slots:
    /**
     * @brief Slot called by the update_timer_ to perform text processing after a short delay.
     * Calls ProcessText() and schedules a repaint.
     */
    void ProcessTextDelayed();

private:
    /**
     * @brief Processes the original_text_ into wrapped lines based on the current widget width.
     * Splits the text into paragraphs and words, then reconstructs lines ensuring they fit
     * within the available width. Handles word wrapping and breaking long words.
     * Updates the processed_lines_ cache, calculates the required height, updates the size_hint_,
     * marks the cache as valid, and emits contentHeightChanged.
     */
    void ProcessText();

signals:
    /**
     * @brief Emitted after ProcessText() finishes, indicating the total calculated height
     * required to display all the processed lines.
     * @param new_height The total content height in pixels.
     */
    void contentHeightChanged(int new_height);

private:
    /** @brief The original, unprocessed text content set via constructor or SetText(). */
    QString original_text_;
    /** @brief List of strings representing the text after processing (line wrapping). */
    QStringList processed_lines_;
    /** @brief The color used to render the text. */
    QColor text_color_;
    /** @brief The font used for rendering text and calculating metrics. */
    QFont font_;
    /** @brief Cached size hint calculated based on processed text dimensions. */
    QSize size_hint_;
    /** @brief Current vertical scroll position in pixels. */
    int scroll_position_;
    /** @brief Flag indicating if the processed_lines_ cache is up-to-date with the original_text_ and widget width. */
    bool cached_text_valid_;
    /** @brief Timer used to delay text processing after resize or text change events. */
    QTimer *update_timer_;
};

#endif // CYBER_LONG_TEXT_DISPLAY_H