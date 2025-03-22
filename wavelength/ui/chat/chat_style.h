#ifndef CHAT_STYLE_H
#define CHAT_STYLE_H

#include <QString>

class ChatStyle {
public:
    static QString getChatStyleSheet();
    static QString getSentMessageStyle();
    static QString getReceivedMessageStyle();
    static QString getSystemMessageStyle();
};

#endif // CHAT_STYLE_H