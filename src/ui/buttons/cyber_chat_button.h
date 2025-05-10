#ifndef CYBER_CHAT_BUTTON_H
#define CYBER_CHAT_BUTTON_H

#include <QPainter>
#include <QPainterPath>
#include <QPushButton>

/**
 * @brief A custom QPushButton styled with a simple cyberpunk aesthetic for chat input areas.
 *
 * This button features a dark blue background, clipped corners, a neon blue border,
 * and neon blue text. The background and border colors change slightly on hover and press.
 * It's designed to be a visually distinct button, often used for sending messages.
 */
class CyberChatButton final : public QPushButton {
    Q_OBJECT

public:
    /**
     * @brief Constructs a CyberChatButton.
     * Initializes the button with the given text and sets the cursor to a pointing hand.
     * @param text The text to display on the button.
     * @param parent Optional parent widget.
     */
    explicit CyberChatButton(const QString &text, QWidget *parent = nullptr);

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom cyberpunk button appearance.
     * Renders the clipped background, neon border, and text, adjusting colors based on
     * hover and press states (isDown(), underMouse()).
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;
};

#endif //CYBER_CHAT_BUTTON_H
