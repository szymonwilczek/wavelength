#ifndef SECURITY_CODE_LAYER_H
#define SECURITY_CODE_LAYER_H

#include <QLabel>
#include <QLineEdit>
#include <QVector>
#include <QList>

#include "../security_layer.h"

/**
 * @brief A security layer requiring the user to enter a 4-character security code.
 *
 * This layer presents four input fields for the user to enter a randomly selected
 * security code (either numeric or alphanumeric). It provides a hint related to the code.
 * Input validation, navigation between fields (using arrows and backspace), and
 * automatic focus progression are handled. Upon correct code entry, the layer fades out
 * and emits the layerCompleted() signal. Incorrect entry triggers a visual error effect.
 */
class SecurityCodeLayer final : public SecurityLayer {
    Q_OBJECT

public:
    /**
     * @brief Constructs a SecurityCodeLayer.
     * Initializes the UI elements (title, instructions, input fields, hint label) and layout.
     * Populates the internal list of possible security codes and hints.
     * @param parent Optional parent widget.
     */
    explicit SecurityCodeLayer(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~SecurityCodeLayer() override;

    /**
     * @brief Initializes the layer for display.
     * Resets the input fields, selects a new random security code and hint, sets appropriate
     * input validators (numeric or alphanumeric), displays the hint, resets opacity, and sets focus
     * to the first input field.
     */
    void Initialize() override;

    /**
     * @brief Resets the layer to its initial state.
     * Clears input fields, resets their styles, clears the hint, resets opacity, and sets focus.
     */
    void Reset() override;

private slots:
    /**
     * @brief Checks if the entered code matches the current security code.
     * Called when all input fields are filled. Triggers success (color change, fade out)
     * or error (visual effect, reset) accordingly.
     */
    void CheckSecurityCode();

    /**
     * @brief Slot called when the text in any input field changes.
     * Handles automatic focus progression to the next field when a character is entered.
     * Converts input to uppercase if the code is alphanumeric.
     * Calls CheckSecurityCode() if all fields become filled.
     */
    void OnDigitEntered();

protected:
    /**
     * @brief Filters events for the input fields to handle navigation keys.
     * Intercepts KeyPress events for QLineEdit objects in code_inputs_.
     * Handles Left/Right arrow keys for focus movement and Backspace for moving focus
     * to the previous field and clearing it if the current field is empty.
     * @param obj The object that generated the event.
     * @param event The event being processed.
     * @return True if the event was handled and should be filtered out, false otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    /**
     * @brief Structure representing a security code and its associated hint.
     */
    struct SecurityCode {
        /** @brief The 4-character security code (can be digits or letters). */
        QString code;
        /** @brief A hint related to the security code. */
        QString hint;
        /** @brief Flag indicating if the code consists only of digits. */
        bool is_numeric;

        /**
         * @brief Constructs a SecurityCode entry.
         * @param c The code string.
         * @param h The hint string.
         * @param num True if the code is numeric only, false otherwise (defaults to true).
         */
        SecurityCode(const QString& c, const QString& h, const bool num = true)
            : code(c), hint(h), is_numeric(num) {}
    };

    /**
     * @brief Selects a random security code from the internal list.
     * @param[out] hint The hint associated with the selected code.
     * @param[out] is_numeric True if the selected code is numeric, false otherwise.
     * @return The randomly selected 4-character security code string.
     */
    QString GetRandomSecurityCode(QString& hint, bool& is_numeric);

    /**
     * @brief Clears all input fields and resets their styles to the default (error/red) state.
     * Sets focus to the first input field.
     */
    void ResetInputs();

    /**
     * @brief Gets the code currently entered by the user by concatenating the text from all input fields.
     * @return The 4-character code string entered by the user.
     */
    QString GetEnteredCode() const;

    /**
     * @brief Triggers a visual error effect on the input fields.
     * Temporarily changes the background and border color of the input fields to indicate an error,
     * then resets the fields and their styles after a short delay.
     */
    void ShowErrorEffect();

    /**
     * @brief Sets the input validators for all code input fields based on whether the current code is numeric.
     * @param numeric_only If true, sets validators to accept only digits [0-9]. If false, accepts digits and uppercase letters [0-9A-Z].
     */
    void SetInputValidators(bool numeric_only);

    /** @brief List containing pointers to the four QLineEdit widgets for code input. */
    QList<QLineEdit*> code_inputs_;
    /** @brief Label displaying the hint for the current security code. */
    QLabel* security_code_hint_;
    /** @brief The currently active security code that the user needs to enter. */
    QString current_security_code_;
    /** @brief Flag indicating if the current_security_code_ is numeric only. */
    bool is_current_code_numeric_;

    /** @brief Vector storing all available SecurityCode entries (code, hint, type). */
    QVector<SecurityCode> security_codes_;
};

#endif // SECURITY_CODE_LAYER_H