#ifndef CHAT_STYLE_H
#define CHAT_STYLE_H

#include <QString>

class ChatStyle {
public:
    // Style dla całego widoku czatu
    static QString GetChatStyleSheet();

    // Style dla dymków wiadomości
    static QString GetSentMessageStyle();
    static QString GetReceivedMessageStyle();
    static QString GetSystemMessageStyle();

    // Style dla komponentów interfejsu czatu
    static QString GetInputFieldStyle();
    static QString GetSendButtonStyle();
    static QString GetAttachButtonStyle();

    // Style dla scrollbara
    static QString GetScrollBarStyle();
};

#endif // CHAT_STYLE_H