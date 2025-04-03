#include "navbar.h"
#include "../button/cyberpunk_button.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QFont>
#include <QContextMenuEvent>
#include <QPushButton>

Navbar::Navbar(QWidget *parent) : QToolBar(parent) {
    setMovable(false);
    setFloatable(false);
    setAllowedAreas(Qt::TopToolBarArea);
    setContextMenuPolicy(Qt::PreventContextMenu);

    // Przezroczyste tło dla navbara
    setStyleSheet(
        "QToolBar {"
        "  background-color: transparent;"
        "  border: none;"
        "  padding: 15px 10px;"
        "  spacing: 15px;"
        "}"
    );

    // Logo - pozostaje bez zmian
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

    // Kontener na przyciski
    QWidget* buttonsContainer = new QWidget(this);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
    buttonsLayout->setContentsMargins(0, 10, 0, 10);
    buttonsLayout->setSpacing(50); // Większy odstęp między przyciskami

    // Utworzenie naszych cyberpunkowych przycisków
    createWavelengthButton = new CyberpunkButton("Generate Wavelength", buttonsContainer);
    joinWavelengthButton = new CyberpunkButton("Merge Wavelength", buttonsContainer);

    // Dodanie przycisków do layoutu
    buttonsLayout->addWidget(createWavelengthButton, 0, Qt::AlignLeft);
    buttonsLayout->addStretch(); // Elastyczna przestrzeń pomiędzy przyciskami
    buttonsLayout->addWidget(joinWavelengthButton, 0, Qt::AlignRight);

    // Dodanie logo i kontenera z przyciskami do paska
    QWidget* spacerLeft = new QWidget();
    spacerLeft->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QWidget* spacerRight = new QWidget();
    spacerRight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Struktura paska: elastyczna przestrzeń - logo - przyciski - elastyczna przestrzeń
    addWidget(spacerLeft);
    addWidget(logoLabel);
    addWidget(buttonsContainer);
    addWidget(spacerRight);

    connect(createWavelengthButton, &QPushButton::clicked, this, &Navbar::createWavelengthClicked);
    connect(joinWavelengthButton, &QPushButton::clicked, this, &Navbar::joinWavelengthClicked);
}

void Navbar::contextMenuEvent(QContextMenuEvent *event) {
    event->ignore();
}