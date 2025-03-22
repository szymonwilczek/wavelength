#include "chat_style.h"

QString ChatStyle::getChatStyleSheet() {
    return QString(
        "QWidget#chatViewContainer {"
        "    background-color: #1a1a1a;"
        "}"
        "QLabel#headerLabel {"
        "    font-size: 16pt;"
        "    color: #a0a0a0;"
        "    margin-bottom: 10px;"
        "    padding: 5px;"
        "    border-bottom: 1px solid #333333;"
        "}"
    );
}

QString ChatStyle::getSentMessageStyle() {
    return QString(
        "background-color: #0b93f6;"
        "border-radius: 18px;"
        "padding: 10px;"
    );
}

QString ChatStyle::getReceivedMessageStyle() {
    return QString(
        "background-color: #262626;"
        "border-radius: 18px;"
        "padding: 10px;"
    );
}

QString ChatStyle::getSystemMessageStyle() {
    return QString(
        "background-color: #3a3a3a;"
        "border-radius: 14px;"
        "padding: 8px;"
    );
}

QString ChatStyle::getInputFieldStyle() {
    return QString(
        "QLineEdit {"
        "    background-color: #333333;"
        "    border: 1px solid #444444;"
        "    border-radius: 5px;"
        "    padding: 8px 12px;"
        "    color: #e0e0e0;"
        "    font-size: 11pt;"
        "    selection-background-color: #4a6db5;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #5a7dc5;"
        "}"
    );
}

QString ChatStyle::getSendButtonStyle() {
    return QString(
        "QPushButton {"
        "    background-color: #4a6db5;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #5a7dc5;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #3a5da5;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #2c3e66;"
        "    color: #aaaaaa;"
        "}"
    );
}

QString ChatStyle::getAttachButtonStyle() {
    return QString(
        "QPushButton {"
        "    background-color: #333333;"
        "    border: 1px solid #444444;"
        "    border-radius: 5px;"
        "    padding: 8px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #444444;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2a2a2a;"
        "}"
    );
}

QString ChatStyle::getScrollBarStyle() {
    return QString(
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