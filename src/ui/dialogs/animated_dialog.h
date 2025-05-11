#ifndef ANIMATED_DIALOG_H
#define ANIMATED_DIALOG_H

#include <QDialog>

class OverlayWidget;

/**
 * @brief A base class for QDialogs that adds animated transitions for showing and closing.
 *
 * This class provides a framework for different animation types (slide, fade, digital materialization)
 * when the dialog appears or disappears. It handles the creation of an optional overlay widget
 * behind the dialog during the transition. Subclasses (like WavelengthDialog, JoinWavelengthDialog)
 * can leverage specific animation types and properties (e.g., digitalizationProgress).
 */
class AnimatedDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Enum defining the available animation styles for showing and closing the dialog.
     */
    enum AnimationType {
        kSlideFromBottom, ///< Dialog slides in from the bottom and out to the bottom.
        kDigitalMaterialization
        ///< Dialog slides in, fades in, and then applies a digital materialization effect (requires specific dialog subclass properties).
    };

    Q_ENUM(AnimationType)

    /**
     * @brief Constructs an AnimatedDialog.
     * Sets necessary window flags and attributes for transparency and animation.
     * @param parent Optional parent widget.
     * @param type The type of animation to use for showing and closing.
     */
    explicit AnimatedDialog(QWidget *parent = nullptr, AnimationType type = kSlideFromBottom);

    /**
     * @brief Destructor. Ensures the overlay widget is cleaned up.
     */
    ~AnimatedDialog() override;

    /**
     * @brief Sets the duration for the show and close animations.
     * @param duration Animation duration in milliseconds.
     */
    void SetAnimationDuration(const int duration) { duration_ = duration; }

    /**
     * @brief Gets the current duration for the show and close animations.
     * @return Animation duration in milliseconds.
     */
    int GetAnimationDuration() const { return duration_; }

signals:
    /**
     * @brief Emitted when the show animation (AnimateShow) has finished.
     */
    void showAnimationFinished();

protected:
    /**
     * @brief Overridden show event handler.
     * Creates and shows the overlay widget (if applicable), then starts the show animation (AnimateShow).
     * @param event The show event.
     */
    void showEvent(QShowEvent *event) override;

    /**
     * @brief Overridden close event handler.
     * Prevents immediate closing, starts the close animation (AnimateClose), and fades out the overlay.
     * The actual QDialog::close() is called when the animation finishes.
     * @param event The close event.
     */
    void closeEvent(QCloseEvent *event) override;

    /** @brief Progress property (0.0 to 1.0) for the digital materialization effect. Animatable. */
    double digitalization_progress_ = 0.0;
    /** @brief Progress property (0.0 to 1.0) potentially for corner glow effects (unused in provided code). Animatable. */
    double corner_glow_progress_ = 0.0;

private:
    /**
     * @brief Initiates and runs the animation sequence for showing the dialog based on animation_type_.
     * Handles different animation types like sliding, fading, and digital materialization.
     * Emits showAnimationFinished() upon completion.
     */
    void AnimateShow();

    /**
     * @brief Initiates and runs the animation sequence for closing the dialog based on animation_type_.
     * Calls QDialog::close() internally when the animation finishes.
     */
    void AnimateClose();

    /** @brief The type of animation selected for this dialog instance. */
    AnimationType animation_type_;
    /** @brief Duration of the show/close animations in milliseconds. */
    int duration_;
    /** @brief Flag to prevent recursive calls during the animated closing process. */
    bool closing_;
    /** @brief Pointer to the semi-transparent overlay widget displayed behind the dialog during animation. */
    OverlayWidget *overlay_;
};

#endif // ANIMATED_DIALOG_H
