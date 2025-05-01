#ifndef CHAT_STYLE_H
#define CHAT_STYLE_H

#include <QString>

/**
 * @brief Provides static methods to retrieve CSS stylesheets for various chat UI elements.
 *
 * This class centralizes the generation of Qt Style Sheets (CSS) used to style
 * the chat view, message bubbles (sent, received, system), input fields, buttons,
 * and scrollbars, ensuring a consistent cyberpunk theme based on CyberpunkStyle colors.
 */
class ChatStyle {
public:
    /**
     * @brief Gets the base stylesheet for the main chat view container and header.
     * Defines background, text color, font, and borders for the overall chat area.
     * @return A QString containing the CSS rules.
     */
    static QString GetChatStyleSheet();

    /**
     * @brief Gets the stylesheet for message bubbles sent by the current user.
     * Defines background, border, padding, margin, color, and font for sent messages.
     * Typically aligned to one side (e.g., left) with a specific color scheme.
     * @return A QString containing the CSS rules.
     */
    static QString GetSentMessageStyle();

    /**
     * @brief Gets the stylesheet for message bubbles received from other users.
     * Defines background, border, padding, margin, color, and font for received messages.
     * Typically aligned to the opposite side of sent messages with a different color scheme.
     * @return A QString containing the CSS rules.
     */
    static QString GetReceivedMessageStyle();

    /**
     * @brief Gets the stylesheet for system messages (e.g., user joined/left, errors).
     * Defines background, border, padding, margin, color, and font for system messages.
     * Typically centered or distinctively styled to differentiate from user messages.
     * @return A QString containing the CSS rules.
     */
    static QString GetSystemMessageStyle();

    /**
     * @brief Gets the stylesheet for the chat message input field (QLineEdit).
     * Defines background, text color, border, padding, and font for the input field,
     * including styles for the focused state.
     * @return A QString containing the CSS rules.
     */
    static QString GetInputFieldStyle();

    /**
     * @brief Gets the stylesheet for the "Send" message button (QPushButton).
     * Defines background, text color, border, padding, and font for the send button,
     * including styles for hover and pressed states.
     * @return A QString containing the CSS rules.
     */
    static QString GetSendButtonStyle();

    /**
     * @brief Gets the stylesheet for the "Attach File" button (QPushButton).
     * Defines background, text color, border, padding, and icon for the attach button,
     * including styles for hover and pressed states.
     * @return A QString containing the CSS rules.
     */
    static QString GetAttachButtonStyle();

    /**
     * @brief Gets the stylesheet for the vertical scrollbar used in the chat view.
     * Defines the appearance of the scrollbar track, handle, and buttons (though buttons are hidden).
     * @return A QString containing the CSS rules.
     */
    static QString GetScrollBarStyle();
};

#endif // CHAT_STYLE_H