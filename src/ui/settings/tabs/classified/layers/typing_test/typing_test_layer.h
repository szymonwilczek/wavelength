#ifndef TYPING_TEST_LAYER_H
#define TYPING_TEST_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QStringList>

/**
 * @brief A security layer implementing a typing verification test.
 *
 * This layer presents the user with a randomly generated text passage (composed of
 * common English words) that they must type accurately. The layer tracks the user's
 * progress, highlighting correctly typed characters in green and the current character
 * with a red background. Input is captured via a hidden QLineEdit. Upon successful
 * completion of the typing test, the UI elements turn green, and the layer fades out,
 * emitting the layerCompleted() signal. Incorrect input is prevented.
 */
class TypingTestLayer final : public SecurityLayer {
    Q_OBJECT

public:
    /**
     * @brief Constructs a TypingTestLayer.
     * Initializes the UI elements (title, instructions, text display panel, hidden input field),
     * sets up the layout, connects the input field's textChanged signal, and generates the initial
     * text passage.
     * @param parent Optional parent widget.
     */
    explicit TypingTestLayer(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~TypingTestLayer() override;

    /**
     * @brief Initializes the layer for display.
     * Resets the layer state, sets focus to the hidden input field, and ensures the layer is fully opaque.
     */
    void Initialize() override;

    /**
     * @brief Resets the layer to its initial state.
     * Clears the input field, resets progress tracking, generates a new text passage, updates the display,
     * resets UI element styles, and ensures the layer is fully opaque.
     */
    void Reset() override;

private slots:
    /**
     * @brief Slot called when the text in the hidden_input_ field changes.
     * Validates the entered text against the target text passage. Updates the display highlighting
     * based on the current correct position. If the entire text is typed correctly, marks the test
     * as completed, updates styles, and triggers the fade-out animation. Prevents incorrect input.
     * @param text The current text in the hidden input field.
     */
    void OnTextChanged(const QString& text);

    /**
     * @brief Updates the text display label after the test is successfully completed.
     * Sets the entire text to green.
     */
    void UpdateHighlight() const;

private:
    /**
     * @brief Generates a random text passage composed of words from a predefined pool.
     * Ensures the generated text fits within the display area (approximately 3 lines)
     * and avoids excessive repetition of words. Stores the generated words in words_
     * and the full passage in full_text_.
     */
    void GenerateWords();

    /**
     * @brief Updates the display_text_label_ with formatted rich text.
     * Highlights correctly typed characters in green, the current character with a red background,
     * and the remaining characters in gray, based on current_position_.
     */
    void UpdateDisplayText() const;

    /** @brief Label displaying the layer title ("TYPING VERIFICATION TEST"). */
    QLabel* title_label_;
    /** @brief Label displaying instructions for the user. */
    QLabel* instructions_label_;
    /** @brief Label displaying the text passage to be typed, with dynamic highlighting. */
    QLabel* display_text_label_;
    /** @brief Hidden input field used to capture user keystrokes without being visible. */
    QLineEdit* hidden_input_;

    /** @brief List of words composing the current text passage. */
    QStringList words_;
    /** @brief The complete text passage the user needs to type. */
    QString full_text_;
    /** @brief Index of the next character the user needs to type correctly in full_text_. */
    int current_position_;
    /** @brief Flag indicating if the user has started typing. */
    bool test_started_;
    /** @brief Flag indicating if the user has successfully completed the typing test. */
    bool test_completed_;
};

#endif // TYPING_TEST_LAYER_H