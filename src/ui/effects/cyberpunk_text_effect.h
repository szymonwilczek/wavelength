#ifndef CYBERPUNK_TEXT_EFFECT_H
#define CYBERPUNK_TEXT_EFFECT_H
#include <QLabel>
#include <QObject>
#include <QTimer>
#include <QRandomGenerator>

/**
 * @brief Applies a multiphase cyberpunk-style text reveal animation to a QLabel.
 *
 * This class takes control of a QLabel and animates its text content through
 * several phases:
 * 1. Scanning: A vertical bar "|" sweeps across the label's width.
 * 2. Typing: The original text is revealed character by character, with occasional random glitch characters inserted.
 * 3. Glitching: A brief final phase where random characters are rapidly inserted and removed from the full text.
 * 4. Final: The animation stops, displaying the original text cleanly.
 *
 * The animation is driven by a QTimer.
 */
class CyberpunkTextEffect final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs a CyberpunkTextEffect.
     * Stores the target QLabel, hides it initially, and sets up the animation timer.
     * @param label Pointer to the QLabel widget to animate.
     * @param parent Optional parent QObject.
     */
    explicit CyberpunkTextEffect(QLabel *label, QWidget *parent = nullptr);

    /**
     * @brief Starts the text reveal an animation sequence from the beginning.
     * Resets the animation phase and character index, then starts the internal timer.
     */
    void StartAnimation();

private slots:
    /**
     * @brief Slot called periodically by the timer to advance the animation.
     * Executes the logic for the current animation phase (scanning, typing, glitching)
     * by calling the corresponding private Animate* method. Stops the timer when
     * the animation completes.
     */
    void NextAnimationStep();

private:
    /**
     * @brief Handles the "Scanning" phase of the animation.
     * Displays a sweeping vertical bar "|" across the label's width.
     * Transitions to the "Typing" phase when complete.
     */
    void AnimateScanning();

    /**
     * @brief Handles the "Typing" phase of the animation.
     * Reveals the original text character by character, inserting random
     * glitch characters occasionally. Transitions to the "Glitching" phase when complete.
     */
    void AnimateTyping();

    /**
     * @brief Handles the final "Glitching" phase of the animation.
     * Briefly displays the full text with rapidly changing random characters inserted.
     * Transitions to the final phase when complete.
     */
    void AnimateGlitching();

    /** @brief Pointer to the QLabel widget being animated. */
    QLabel *label_;
    /** @brief The original text content of the label. */
    QString original_text_;
    /** @brief Timer driving the animation steps. */
    QTimer *timer_;
    /** @brief Current phase of the animation (0: Scan, 1: Type, 2: Glitch, 3: Done). */
    int phase_ = 0;
    /** @brief Index used during scanning and typing phases to track progress. */
    int char_index_ = 0;
    /** @brief Counter used during the final glitching phase. */
    int glitch_count_ = 0;
};

#endif //CYBERPUNK_TEXT_EFFECT_H
