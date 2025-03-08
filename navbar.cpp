#include "navbar.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QFont>
#include <QContextMenuEvent>

Navbar::Navbar(QWidget *parent) : QToolBar(parent) {
   
    setMovable(false);
    setFloatable(false);
    setAllowedAreas(Qt::TopToolBarArea);

   
    setContextMenuPolicy(Qt::PreventContextMenu);

   
    setStyleSheet(
        "QToolBar {"
        "  background-color: #2a2a2a;"
        "  border-bottom: 1px solid #3a3a3a;"
        "  padding: 6px 10px;"
        "  spacing: 10px;"
        "}"
    );

   
    logoLabel = new QLabel("Pk4", this);
    QFont logoFont("Arial", 14);
    logoFont.setStyleStrategy(QFont::PreferAntialias);
    logoLabel->setFont(logoFont);
    logoLabel->setStyleSheet(
        "QLabel {"
        "  color: #f0f0f0;"
        "  font-weight: bold;"
        "  padding-right: 20px;"
        "}"
    );

   
    QString buttonStyle =
        "QPushButton {"
        "  background-color: #3e3e3e;"
        "  color: #e0e0e0;"
        "  border: 1px solid #4a4a4a;"
        "  border-radius: 4px;"
        "  padding: 8px 16px;"
        "  font-family: 'Segoe UI', Arial, sans-serif;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: #4a4a4a;"
        "  border: 1px solid #5a5a5a;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #2e2e2e;"
        "  border: 1px solid #555555;"
        "}";

   
    createWavelengthButton = new QPushButton("Generate Wavelength", this);
    joinWavelengthButton = new QPushButton("Merge Wavelength", this);

   
    createWavelengthButton->setStyleSheet(buttonStyle);
    joinWavelengthButton->setStyleSheet(buttonStyle);

   
    QFont buttonFont("Segoe UI", 9);
    buttonFont.setStyleStrategy(QFont::PreferAntialias);
    createWavelengthButton->setFont(buttonFont);
    joinWavelengthButton->setFont(buttonFont);

   
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    addWidget(logoLabel);
    addWidget(createWavelengthButton);
    addWidget(joinWavelengthButton);
    addWidget(spacer);

    connect(createWavelengthButton, &QPushButton::clicked, this, &Navbar::createWavelengthClicked);
    connect(joinWavelengthButton, &QPushButton::clicked, this, &Navbar::joinWavelengthClicked);
}

void Navbar::contextMenuEvent(QContextMenuEvent *event) {
   
    event->ignore();
}