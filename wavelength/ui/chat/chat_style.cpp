#include "chat_style.h"

#include "../cyberpunk_style.h"


QString ChatStyle::getChatStyleSheet() {
    return QString(
                "QWidget#chatViewContainer {"
                "  background-color: %1;"
                "  color: %2;"
                "  font-family: 'Consolas';"
                "  border: 1px solid %3;"
                "}"
                "QLabel#headerLabel {"
                "  font-size: 16px;"
                "  font-weight: bold;"
                "  color: %4;"
                "  background-color: %5;"
                "  border: 1px solid %6;"
                "  padding: 8px 12px;"
                "  border-radius: 0px;"
                "}"
            )
            .arg(CyberpunkStyle::bgDarkColor().name())
            .arg(CyberpunkStyle::primaryColor().name())
            .arg(CyberpunkStyle::primaryColor().name())
            .arg(CyberpunkStyle::secondaryColor().name())
            .arg(CyberpunkStyle::bgDarkColor().darker(120).name())
            .arg(CyberpunkStyle::primaryColor().name());
}

QString ChatStyle::getSentMessageStyle() {
    return QString(
                "background-color: rgba(0, 60, 80, 180);"
                "border: 1px solid %1;"
                "border-radius: 3px;"
                "padding: 8px 12px;"
                "margin: 2px 10% 2px 5px;"
                "color: #e0ffff;"
                "font-family: 'Consolas';"
            )
            .arg(CyberpunkStyle::primaryColor().name());
}

QString ChatStyle::getReceivedMessageStyle() {
    return QString(
                "background-color: rgba(40, 20, 60, 180);"
                "border: 1px solid %1;"
                "border-radius: 3px;"
                "padding: 8px 12px;"
                "margin: 2px 5px 2px 10%;"
                "color: #f0e0ff;"
                "font-family: 'Consolas';"
            )
            .arg(CyberpunkStyle::accentColor().name());
}

QString ChatStyle::getSystemMessageStyle() {
    return QString(
                "background-color: rgba(40, 40, 10, 180);"
                "border: 1px solid %1;"
                "border-radius: 3px;"
                "padding: 5px 10px;"
                "margin: 2px 20% 2px 20%;"
                "color: #ffee88;"
                "font-family: 'Consolas';"
                "font-size: 90%;"
            )
            .arg(CyberpunkStyle::warningColor().name());
}

QString ChatStyle::getInputFieldStyle() {
    return QString(
                "QLineEdit {"
                "  background-color: rgba(0, 30, 40, 200);"
                "  color: %1;"
                "  border: 1px solid %2;"
                "  border-radius: 3px;"
                "  padding: 8px;"
                "  font-family: 'Consolas';"
                "  font-size: 14px;"
                "}"
                "QLineEdit:focus {"
                "  border: 1px solid %3;"
                "  background-color: rgba(0, 35, 45, 220);"
                "}"
            )
            .arg(CyberpunkStyle::secondaryColor().name())
            .arg(CyberpunkStyle::primaryColor().name())
            .arg(CyberpunkStyle::secondaryColor().name());
}

QString ChatStyle::getSendButtonStyle() {
    return QString(
                "QPushButton {"
                "  background-color: #004466;"
                "  color: %1;"
                "  border: 1px solid %2;"
                "  border-radius: 3px;"
                "  padding: 8px 16px;"
                "  font-family: 'Consolas';"
                "  font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "  background-color: #005577;"
                "  border: 1px solid %3;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #003344;"
                "}"
            )
            .arg(CyberpunkStyle::secondaryColor().name())
            .arg(CyberpunkStyle::primaryColor().name())
            .arg(CyberpunkStyle::secondaryColor().name());
}

QString ChatStyle::getAttachButtonStyle() {
    return QString(
                "QPushButton {"
                "  background-color: #004466;"
                "  color: %1;"
                "  border: 1px solid %2;"
                "  border-radius: 3px;"
                "  padding: 4px;"
                "  qproperty-icon: url(:/icons/attach-icon.png);"
                "  qproperty-iconSize: 20px 20px;"
                "}"
                "QPushButton:hover {"
                "  background-color: #005577;"
                "  border: 1px solid %3;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #003344;"
                "}"
            )
            .arg(CyberpunkStyle::secondaryColor().name())
            .arg(CyberpunkStyle::primaryColor().name())
            .arg(CyberpunkStyle::secondaryColor().name());
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
