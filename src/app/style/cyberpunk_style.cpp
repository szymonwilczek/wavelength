#include "cyberpunk_style.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

void CyberpunkStyle::applyStyle() {
        QApplication::setStyle(QStyleFactory::create("Fusion"));

        // Ustaw podstawową paletę cyberpunkową
        QPalette palette;
        palette.setColor(QPalette::Window, bgDarkColor());
        palette.setColor(QPalette::WindowText, textColor());
        palette.setColor(QPalette::Base, QColor(15, 25, 35));
        palette.setColor(QPalette::AlternateBase, QColor(25, 40, 55));
        palette.setColor(QPalette::ToolTipBase, QColor(15, 25, 35));
        palette.setColor(QPalette::ToolTipText, textColor());
        palette.setColor(QPalette::Text, textColor());
        palette.setColor(QPalette::Button, bgMediumColor());
        palette.setColor(QPalette::ButtonText, textColor());
        palette.setColor(QPalette::Link, primaryColor());
        palette.setColor(QPalette::Highlight, primaryColor());
        palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
        palette.setColor(QPalette::Disabled, QPalette::Text, mutedTextColor());
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, mutedTextColor());

        QApplication::setPalette(palette);

        // Zastosuj globalny stylesheet dla standardowych kontrolek
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
                .arg(bgMediumColor().name()) // tło przycisku
                .arg(textColor().name()) // kolor tekstu przycisku
                .arg(primaryColor().darker(120).name()) // obramowanie przycisku
                .arg(bgLightColor().name()) // tło przycisku hover
                .arg(primaryColor().name()) // obramowanie hover
                .arg(bgDarkColor().name()) // tło wciśniętego przycisku
                .arg(QColor(10, 15, 25).name()) // tło pola tekstowego
                .arg(textColor().name()) // kolor tekstu
                .arg(bgMediumColor().darker(120).name()) // obramowanie pola
                .arg(primaryColor().name()) // obramowanie z focusem
                .arg(QColor(10, 15, 25).name()) // tło scrollbara
                .arg(primaryColor().darker(120).name()) // uchwyt scrollbara
                .arg(bgDarkColor().darker(110).name()) // tło menu/paska
                .arg(textColor().name()) // tekst menu
                .arg(bgMediumColor().darker(120).name()) // obramowanie menu
                .arg(bgLightColor().name()) // tło wybranego menu
                .arg(textColor().name()) // kolor tekstu tooltip
                .arg(bgDarkColor().name()) // tło tooltip
                .arg(primaryColor().darker(120).name()) // obramowanie tooltip
                .arg(bgDarkColor().name()); // tło dialogu

        qApp->setStyleSheet(stylesheet);
    }

QString CyberpunkStyle::getTechBorderStyle(const bool isActive) {
    const QString color = isActive ? primaryColor().name() : primaryColor().darker(150).name();
    return QString(
        "border: 1px solid %1;"
        "border-radius: 0px;"
    ).arg(color);
}

QString CyberpunkStyle::getCyberpunkFrameStyle() {
    return QString(
                "background-color: %1;"
                "border: 1px solid %2;"
                "border-radius: 3px;"
            )
            .arg(bgDarkColor().name(), primaryColor().name());
}
