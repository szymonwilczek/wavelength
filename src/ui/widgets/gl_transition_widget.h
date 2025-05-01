#ifndef GL_TRANSITION_WIDGET_H
#define GL_TRANSITION_WIDGET_H

#include <QOpenGLWidget>
#include <QPixmap>
#include <QPropertyAnimation>

/**
 * @brief An OpenGL widget designed to render smooth, animated slide transitions between two other widgets.
 *
 * This widget captures the visual representation (pixmap) of the current and next widgets
 * and then uses QPainter (within the OpenGL context) to draw them sliding horizontally.
 * The transition progress is controlled by a QPropertyAnimation animating the 'offset' property.
 * It's intended to be used by AnimatedStackedWidget for hardware-accelerated slide transitions.
 */
class GLTransitionWidget final : public QOpenGLWidget
{
    Q_OBJECT
    /** @brief Property controlling the horizontal offset during the transition (0.0 to 1.0). Animatable. */
    Q_PROPERTY(float offset READ GetOffset WRITE SetOffset)

public:
    /**
     * @brief Constructs a GLTransitionWidget.
     * Initializes the QPropertyAnimation for the offset and sets widget attributes for optimal painting.
     * @param parent Optional parent widget.
     */
    explicit GLTransitionWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Cleans up the QPropertyAnimation.
     */
    ~GLTransitionWidget() override;

    /**
     * @brief Sets the widgets involved in the transition.
     * Captures the current visual state of the provided widgets into pixmaps (current_pixmap_, next_pixmap_)
     * which will be used for rendering the animation. Takes device pixel ratio into account.
     * @param current_widget The widget currently visible, which will slide out.
     * @param next_widget The widget that will slide in.
     */
    void SetWidgets(QWidget *current_widget, QWidget *next_widget);

    /**
     * @brief Starts the slide transition animation.
     * Configures and starts the QPropertyAnimation to animate the 'offset' property from 0.0 to 1.0
     * over the specified duration.
     * @param duration The duration of the transition in milliseconds.
     */
    void StartTransition(int duration);

    /**
     * @brief Gets the current transition offset.
     * @return The current offset value (0.0 to 1.0).
     */
    float GetOffset() const { return offset_; }

    /**
     * @brief Sets the transition offset.
     * This is typically called by the QPropertyAnimation. Updates the internal offset_ value
     * and schedules a repaint of the widget.
     * @param offset The new offset value (0.0 to 1.0).
     */
    void SetOffset(float offset);

signals:
    /**
     * @brief Emitted when the transition animation (QPropertyAnimation) finishes.
     */
    void transitionFinished();

protected:
    /**
     * @brief Overridden paint event handler. Renders the transition frame.
     * Draws the current_pixmap_ sliding out to the left and the next_pixmap_ sliding in
     * from the right, based on the current value of offset_. Uses QPainter with high-quality settings.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /** @brief Pixmap captured from the widget that is sliding out. */
    QPixmap current_pixmap_;
    /** @brief Pixmap captured from the widget that is sliding in. */
    QPixmap next_pixmap_;
    /** @brief Current progress of the transition animation (0.0 = start, 1.0 = end). */
    float offset_ = 0.0f;
    /** @brief Animation object controlling the 'offset' property over time. */
    QPropertyAnimation *animation_ = nullptr;
};

#endif // GL_TRANSITION_WIDGET_H