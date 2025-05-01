#ifndef ANIMATED_STACKED_WIDGET_H
#define ANIMATED_STACKED_WIDGET_H

#include <QStackedWidget>
#include <QParallelAnimationGroup>
#include <QSoundEffect>

#include "gl_transition_widget.h"

/**
 * @brief A QStackedWidget subclass that provides animated transitions between widgets.
 *
 * This widget enhances the standard QStackedWidget by adding visual animations
 * (like fade, slide, push) when switching between child widgets. It supports
 * different animation types and durations, and can optionally use OpenGL for
 * smoother slide transitions via GLTransitionWidget. It also plays a sound effect
 * during transitions.
 */
class AnimatedStackedWidget final : public QStackedWidget
{
    Q_OBJECT
    /** @brief Property controlling the duration of the transition animation in milliseconds. */
    Q_PROPERTY(int duration READ GetDuration WRITE SetDuration)

public:
    /**
     * @brief Enum defining the available types of transition animations.
     */
    enum AnimationType {
        Fade,           ///< Cross-fade between the current and next widget.
        Slide,          ///< Slide the current widget out and the next widget in (can use OpenGL).
        SlideAndFade,   ///< Combine sliding and fading effects.
        Push            ///< Push the current widget partially out while the next slides in.
    };

    /**
     * @brief Constructs an AnimatedStackedWidget.
     * Initializes the animation group, sound effect, and the optional OpenGL transition widget.
     * Connects animation signals.
     * @param parent Optional parent widget.
     */
    explicit AnimatedStackedWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Cleans up the animation group and the OpenGL transition widget.
     */
    ~AnimatedStackedWidget() override;

    // --- Settings ---
    /**
     * @brief Sets the duration of the transition animation.
     * @param duration The animation duration in milliseconds.
     */
    void SetDuration(const int duration) { duration_ = duration; }

    /**
     * @brief Gets the current duration of the transition animation.
     * @return The animation duration in milliseconds.
     */
    int GetDuration() const { return duration_; }

    /**
     * @brief Sets the type of animation to use for transitions.
     * @param type The desired AnimationType.
     */
    void SetAnimationType(const AnimationType type) { animation_type_ = type; }

    /**
     * @brief Gets the currently configured animation type.
     * @return The current AnimationType.
     */
    AnimationType GetAnimationType() const { return animation_type_; }

    /**
     * @brief Checks if a transition animation is currently running.
     * @return True if an animation is in progress, false otherwise.
     */
    bool IsAnimating() const { return animation_running_; }
    // --- End Settings ---

public slots:
    /**
     * @brief Initiates an animated transition to the widget at the specified index.
     * Plays a sound effect and starts the configured animation type. Does nothing if
     * an animation is already running or the index is invalid/current.
     * @param index The index of the target widget.
     */
    void SlideToIndex(int index);

    /**
     * @brief Initiates an animated transition to the specified widget.
     * Convenience slot that finds the index of the widget and calls SlideToIndex().
     * @param widget Pointer to the target widget.
     */
    void SlideToWidget(QWidget *widget);

    /**
     * @brief Initiates an animated transition to the next widget in the stack (wraps around).
     * Useful for creating carousel-like behavior.
     */
    void SlideToNextIndex();

protected:
    /**
     * @brief Configures and adds animations to the animation_group_ for a fade transition.
     * @param next_index The index of the target widget.
     */
    void AnimateFade(int next_index) const;

    /**
     * @brief Configures and adds animations to the animation_group_ for a slide transition (CPU fallback).
     * @param next_index The index of the target widget.
     */
    void AnimateSlide(int next_index) const;

    /**
     * @brief Configures and adds animations to the animation_group_ for a combined slide and fade transition.
     * @param next_index The index of the target widget.
     */
    void AnimateSlideAndFade(int next_index) const;

    /**
     * @brief Configures and adds animations to the animation_group_ for a push transition.
     * @param next_index The index of the target widget.
     */
    void AnimatePush(int next_index) const;

private slots:
    /**
     * @brief Slot called when the OpenGL transition animation finishes.
     * Hides the OpenGL widget, updates the current index, shows the target widget,
     * resets animation flags, and emits currentChanged().
     */
    void OnGLTransitionFinished();

private:
    /**
     * @brief Prepares the appropriate animation based on animation_type_.
     * Calls the corresponding Animate* method or sets up the OpenGL transition.
     * Starts the animation_group_ if not using OpenGL.
     * @param next_index The index of the target widget.
     */
    void PrepareAnimation(int next_index) const;

    /**
     * @brief Cleans up graphics effects and resets widget geometries after an animation finishes.
     * Ensures widgets are in their correct final state and removes temporary effects.
     */
    void CleanupAfterAnimation() const;

    /** @brief Duration of the transition animation in milliseconds. */
    int duration_;
    /** @brief The type of animation currently configured. */
    AnimationType animation_type_;
    /** @brief Parallel animation group managing the individual property animations. */
    QParallelAnimationGroup *animation_group_;
    /** @brief Flag indicating if a transition animation is currently running. */
    bool animation_running_;
    /** @brief Stores the index of the target widget during an animation. */
    int target_index_;
    /** @brief Optional widget for handling OpenGL-accelerated slide transitions. */
    GLTransitionWidget *gl_transition_widget_ = nullptr;
    /** @brief Sound effect played during transitions. */
    QSoundEffect *swoosh_sound_;
};

#endif // ANIMATED_STACKED_WIDGET_H