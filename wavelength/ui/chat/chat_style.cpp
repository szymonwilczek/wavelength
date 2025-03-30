#include "chat_style.h"

QString ChatStyle::getChatStyleSheet() {
    return QString(
        "QWidget#chatViewContainer {"
        "  background-color: #0a1520;"
        "  color: #00ccff;"
        "  font-family: 'Consolas';"
        "  border: 1px solid #00aaff;"
        "}"
        "QLabel#headerLabel {"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  color: #00ffff;"
        "  background-color: #001822;"
        "  border: 1px solid #00aaff;"
        "  padding: 8px 12px;"
        "  border-radius: 0px;"
        "}"
    );
}

QString ChatStyle::getSentMessageStyle() {
    // Cyberpunkowy styl dla wysłanych wiadomości
    return QString(
        "background-color: rgba(0, 60, 80, 180);"
        "border: 1px solid #00aaff;"
        "border-radius: 3px;"
        "padding: 8px 12px;"
        "margin: 2px 10% 2px 5px;"
        "color: #e0ffff;"
        "font-family: 'Consolas';"
    );
}

QString ChatStyle::getReceivedMessageStyle() {
    // Cyberpunkowy styl dla otrzymanych wiadomości
    return QString(
        "background-color: rgba(40, 20, 60, 180);"
        "border: 1px solid #aa60ff;"
        "border-radius: 3px;"
        "padding: 8px 12px;"
        "margin: 2px 5px 2px 10%;"
        "color: #f0e0ff;"
        "font-family: 'Consolas';"
    );
}

QString ChatStyle::getSystemMessageStyle() {
    // Cyberpunkowy styl dla wiadomości systemowych
    return QString(
        "background-color: rgba(40, 40, 10, 180);"
        "border: 1px solid #ffaa00;"
        "border-radius: 3px;"
        "padding: 5px 10px;"
        "margin: 2px 20% 2px 20%;"
        "color: #ffee88;"
        "font-family: 'Consolas';"
        "font-size: 90%;"
    );
}

QString ChatStyle::getInputFieldStyle() {
    return QString(
        "QLineEdit {"
        "  background-color: rgba(0, 30, 40, 200);"
        "  color: #00ffff;"
        "  border: 1px solid #00aaff;"
        "  border-radius: 3px;"
        "  padding: 8px;"
        "  font-family: 'Consolas';"
        "  font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #00ddff;"
        "  background-color: rgba(0, 35, 45, 220);"
        "}"
    );
}

QString ChatStyle::getSendButtonStyle() {
    return QString(
        "QPushButton {"
        "  background-color: #004466;"
        "  color: #00ddff;"
        "  border: 1px solid #00aaff;"
        "  border-radius: 3px;"
        "  padding: 8px 16px;"
        "  font-family: 'Consolas';"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #005577;"
        "  border: 1px solid #00ddff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #003344;"
        "}"
    );
}

QString ChatStyle::getAttachButtonStyle() {
    return QString(
        "QPushButton {"
        "  background-color: #004466;"
        "  color: #00ddff;"
        "  border: 1px solid #00aaff;"
        "  border-radius: 3px;"
        "  padding: 4px;"
        "  qproperty-icon: url(:/icons/attach-icon.png);"
        "  qproperty-iconSize: 20px 20px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #005577;"
        "  border: 1px solid #00ddff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #003344;"
        "}"
    );
}

QString ChatStyle::getScrollBarStyle() {
    return QString(
        "QScrollBar:vertical {"
        "  background: rgba(0, 20, 30, 100);"
        "  width: 12px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(0, 150, 220, 150);"
        "  min-height: 20px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: none;"
        "}"
    );
}
