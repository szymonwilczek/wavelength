#include "chat_style.h"

QString ChatStyle::getChatStyleSheet() {
    return QString(
        "QScrollArea { "
        "    background-color: #1b1b1b; "
        "    border: none; "
        "}"
        "QScrollBar:vertical {"
        "    border: none;"
        "    background: rgba(45, 45, 45, 100);"
        "    width: 8px;"
        "    margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: rgba(120, 120, 120, 120);"
        "    min-height: 20px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "    background: none;"
        "}"
    );
}

QString ChatStyle::getSentMessageStyle() {
    return QString(
        "background-color: #0b93f6; "
        "border-radius: 18px; "
        "color: white; "
        "padding: 10px;"
    );
}

QString ChatStyle::getReceivedMessageStyle() {
    return QString(
        "background-color: #262626; "
        "border-radius: 18px; "
        "color: white; "
        "padding: 10px;"
    );
}

QString ChatStyle::getSystemMessageStyle() {
    return QString(
        "background-color: #3a3a3a; "
        "border-radius: 14px; "
        "color: #ffcc00; "
        "padding: 8px;"
    );
}