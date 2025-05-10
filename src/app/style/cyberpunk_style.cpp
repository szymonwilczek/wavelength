#include "cyberpunk_style.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

void CyberpunkStyle::ApplyStyle() {
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, GetBackgroundDarkColor());
    palette.setColor(QPalette::WindowText, GetTextColor());
    palette.setColor(QPalette::Base, QColor(15, 25, 35));
    palette.setColor(QPalette::AlternateBase, QColor(25, 40, 55));
    palette.setColor(QPalette::ToolTipBase, QColor(15, 25, 35));
    palette.setColor(QPalette::ToolTipText, GetTextColor());
    palette.setColor(QPalette::Text, GetTextColor());
    palette.setColor(QPalette::Button, GetBackgroundMediumColor());
    palette.setColor(QPalette::ButtonText, GetTextColor());
    palette.setColor(QPalette::Link, GetPrimaryColor());
    palette.setColor(QPalette::Highlight, GetPrimaryColor());
    palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    palette.setColor(QPalette::Disabled, QPalette::Text, GetMutedTextColor());
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, GetMutedTextColor());

    QApplication::setPalette(palette);

    const QString stylesheet = QString(
                // general settings
                "QWidget {"
                "  font-family: 'Consolas', 'Courier New', monospace;"
                "}"

                // buttons
                "QPushButton {"
                "  background-color: %1;"
                "  color: %2;"
                "  border: 1px solid %3;"
                "  border-radius: 3px;"
                "  padding: 5px 15px;"
                "}"

                "QPushButton:hover {"
                "  background-color: %4;"
                "  border: 1px solid %5;"
                "}"

                "QPushButton:pressed {"
                "  background-color: %6;"
                "}"

                // inputs
                "QLineEdit, QTextEdit, QPlainTextEdit {"
                "  background-color: %7;"
                "  color: %8;"
                "  border: 1px solid %9;"
                "  border-radius: 3px;"
                "  padding: 2px 5px;"
                "}"

                "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {"
                "  border: 1px solid %10;"
                "}"

                // scrollbars
                "QScrollBar:vertical {"
                "  background: %11;"
                "  width: 12px;"
                "  margin: 0px;"
                "}"

                "QScrollBar::handle:vertical {"
                "  background: %12;"
                "  min-height: 20px;"
                "  border-radius: 3px;"
                "}"

                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
                "  height: 0px;"
                "}"

                // menu & toolbars
                "QMenuBar, QToolBar {"
                "  background-color: %13;"
                "  color: %14;"
                "  border-bottom: 1px solid %15;"
                "}"

                "QMenuBar::item:selected, QMenu::item:selected {"
                "  background-color: %16;"
                "}"

                // tooltips
                "QToolTip {"
                "  color: %17;"
                "  background-color: %18;"
                "  border: 1px solid %19;"
                "  padding: 2px;"
                "}"

                // dialogs
                "QDialog {"
                "  background-color: %20;"
                "}"
            )
            .arg(GetBackgroundMediumColor().name()) // button background
            .arg(GetTextColor().name()) // button text color
            .arg(GetPrimaryColor().darker(120).name()) // button border
            .arg(GetBackgroundLightColor().name()) // button background:hover
            .arg(GetPrimaryColor().name()) // button border:hover
            .arg(GetBackgroundDarkColor().name()) // button:pressed background
            .arg(QColor(10, 15, 25).name()) // input background
            .arg(GetTextColor().name()) // text color
            .arg(GetBackgroundMediumColor().darker(120).name()) // input border
            .arg(GetPrimaryColor().name()) // input border:focus
            .arg(QColor(10, 15, 25).name()) // scrollbar background
            .arg(GetPrimaryColor().darker(120).name()) // scrollbar handle
            .arg(GetBackgroundDarkColor().darker(110).name()) // menu/toolbar background
            .arg(GetTextColor().name()) // menu text
            .arg(GetBackgroundMediumColor().darker(120).name()) // menu border
            .arg(GetBackgroundLightColor().name()) // selected menu background
            .arg(GetTextColor().name()) // tooltip text color
            .arg(GetBackgroundDarkColor().name()) // tooltip background
            .arg(GetPrimaryColor().darker(120).name()) // tooltip border
            .arg(GetBackgroundDarkColor().name()); // dialog background

    qApp->setStyleSheet(stylesheet);
}

QString CyberpunkStyle::GetTechBorderStyle(const bool is_active) {
    const QString color = is_active ? GetPrimaryColor().name() : GetPrimaryColor().darker(150).name();
    return QString(
        "border: 1px solid %1;"
        "border-radius: 0px;"
    ).arg(color);
}

QString CyberpunkStyle::GetCyberpunkFrameStyle() {
    return QString(
                "background-color: %1;"
                "border: 1px solid %2;"
                "border-radius: 3px;"
            )
            .arg(GetBackgroundDarkColor().name(), GetPrimaryColor().name());
}
