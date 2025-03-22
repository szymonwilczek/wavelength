#ifndef CHAT_STYLE_H
#define CHAT_STYLE_H

#include <QString>

class ChatStyle {
public:
    // Style dla całego widoku czatu
    static QString getChatStyleSheet();

    // Style dla dymków wiadomości
    static QString getSentMessageStyle();
    static QString getReceivedMessageStyle();
    static QString getSystemMessageStyle();

    // Style dla komponentów interfejsu czatu
    static QString getInputFieldStyle();
    static QString getSendButtonStyle();
    static QString getAttachButtonStyle();

    // Style dla scrollbara
    static QString getScrollBarStyle();
};

#endif // CHAT_STYLE_H