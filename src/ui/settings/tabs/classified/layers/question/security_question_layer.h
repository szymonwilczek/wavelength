#ifndef SECURITY_QUESTION_LAYER_H
#define SECURITY_QUESTION_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QLineEdit>
#include <QTimer>

/**
 * @brief A security layer simulating a security question challenge.
 *
 * This layer presents a generic security question prompt and an input field.
 * It includes a timer that reveals a hint after a delay, suggesting the user
 * doesn't actually need a question if they are legitimate.
 * In this implementation, any answer entered by the user is considered correct
 * as part of a joke/Easter egg. Upon "successful" authentication (any input followed by Enter),
 * the UI elements turn green, and the layer fades out, emitting the layerCompleted() signal.
 */
class SecurityQuestionLayer final : public SecurityLayer {
    Q_OBJECT

public:
    /**
     * @brief Constructs a SecurityQuestionLayer.
     * Initializes the UI elements (title, instructions, question label, input field),
     * sets up the layout, and creates the hint timer. Connects the input field's
     * returnPressed signal to the checking slot.
     * @param parent Optional parent widget.
     */
    explicit SecurityQuestionLayer(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops and deletes the hint timer.
     */
    ~SecurityQuestionLayer() override;

    /**
     * @brief Initializes the layer for display.
     * Resets the input field and styles, sets the initial question text, starts the hint timer,
     * sets focus to the input field, and ensures the layer is fully opaque.
     */
    void Initialize() override;

    /**
     * @brief Resets the layer to its initial state.
     * Stops the hint timer, clears the input field and question label, resets styles,
     * makes the input field editable, and ensures the layer is fully opaque.
     */
    void Reset() override;

private slots:
    /**
     * @brief Checks the security answer (always accepts in this implementation).
     * Called when the user presses Enter in the input field. Changes UI colors to green,
     * updates the label text, and initiates the fade-out animation after a short delay.
     */
    void CheckSecurityAnswer();

    /**
     * @brief Slot called by security_question_timer_ after a timeout (10 seconds).
     * Updates the security_question_label_ with a hint message.
     */
    void SecurityQuestionTimeout() const;

private:
    /** @brief Label displaying the security question prompt or hint. */
    QLabel* security_question_label_;
    /** @brief Input field for the user to type their answer. */
    QLineEdit* security_question_input_;
    /** @brief Timer controlling when the hint is displayed. */
    QTimer* security_question_timer_;
};

#endif // SECURITY_QUESTION_LAYER_H