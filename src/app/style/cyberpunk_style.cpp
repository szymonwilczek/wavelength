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
                    // Ustawienia ogólne
                    "QWidget {"
                    "  font-family: 'Consolas', 'Courier New', monospace;"
                    "}"

                    // Przyciski
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

                    // Pola wprowadzania
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

                    // Paski przewijania
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

                    // Menu i paski narzędzi
                    "QMenuBar, QToolBar {"
                    "  background-color: %13;"
                    "  color: %14;"
                    "  border-bottom: 1px solid %15;"
                    "}"

                    "QMenuBar::item:selected, QMenu::item:selected {"
                    "  background-color: %16;"
                    "}"

                    // Tooltip
                    "QToolTip {"
                    "  color: %17;"
                    "  background-color: %18;"
                    "  border: 1px solid %19;"
                    "  padding: 2px;"
                    "}"

                    // Nakładki dialogowe
                    "QDialog {"
                    "  background-color: %20;"
                    "}"
                )
                .arg(GetBackgroundMediumColor().name()) // tło przycisku
                .arg(GetTextColor().name()) // kolor tekstu przycisku
                .arg(GetPrimaryColor().darker(120).name()) // obramowanie przycisku
                .arg(GetBackgroundLightColor().name()) // tło przycisku hover
                .arg(GetPrimaryColor().name()) // obramowanie hover
                .arg(GetBackgroundDarkColor().name()) // tło wciśniętego przycisku
                .arg(QColor(10, 15, 25).name()) // tło pola tekstowego
                .arg(GetTextColor().name()) // kolor tekstu
                .arg(GetBackgroundMediumColor().darker(120).name()) // obramowanie pola
                .arg(GetPrimaryColor().name()) // obramowanie z focusem
                .arg(QColor(10, 15, 25).name()) // tło scrollbara
                .arg(GetPrimaryColor().darker(120).name()) // uchwyt scrollbara
                .arg(GetBackgroundDarkColor().darker(110).name()) // tło menu/paska
                .arg(GetTextColor().name()) // tekst menu
                .arg(GetBackgroundMediumColor().darker(120).name()) // obramowanie menu
                .arg(GetBackgroundLightColor().name()) // tło wybranego menu
                .arg(GetTextColor().name()) // kolor tekstu tooltip
                .arg(GetBackgroundDarkColor().name()) // tło tooltip
                .arg(GetPrimaryColor().darker(120).name()) // obramowanie tooltip
                .arg(GetBackgroundDarkColor().name()); // tło dialogu

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
