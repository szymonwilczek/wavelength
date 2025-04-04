#include "navbar.h"
#include "../button/cyberpunk_button.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QFont>
#include <QContextMenuEvent>
#include <QPushButton>
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>

Navbar::Navbar(QWidget *parent) : QToolBar(parent) {
    setMovable(false);
    setFloatable(false);
    setAllowedAreas(Qt::TopToolBarArea);
    setContextMenuPolicy(Qt::PreventContextMenu);

    // Gradient tła dla navbaru z transparentnością
    setStyleSheet(
        "QToolBar {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "                           stop:0 rgba(0, 15, 30, 220),"
        "                           stop:1 rgba(10, 25, 40, 200));"
        "  border: none;"
        "  border-bottom: 1px solid rgba(0, 195, 255, 70);"
        "  padding: 15px 20px;"
        "  spacing: 0px;"
        "}"
    );

    // Główny kontener z layoutem horyzontalnym
    QWidget* mainContainer = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Logo z neonowym efektem poświaty - przyciągnięte do lewej
    logoLabel = new QLabel("WAVELENGTH", this);
    QFont logoFont("Blender Pro", 12);
    logoFont.setWeight(QFont::Bold);
    logoFont.setStyleStrategy(QFont::PreferAntialias);
    logoLabel->setFont(logoFont);
    logoLabel->setStyleSheet(
        "QLabel {"
        "  color: #f0f0f0;"
        "  padding: 5px 0px;"
        "  margin-right: 20px;"
        "}"
    );

    // Dodanie neonowego efektu poświaty
    QGraphicsDropShadowEffect* textGlow = new QGraphicsDropShadowEffect(logoLabel);
    textGlow->setBlurRadius(8);
    textGlow->setOffset(0, 0);
    textGlow->setColor(QColor(0, 195, 255, 150));
    logoLabel->setGraphicsEffect(textGlow);

    // Dodanie elementu narożnego do logo
    QLabel* cornerElement1 = new QLabel(this);
    cornerElement1->setStyleSheet(
        "QLabel {"
        "  color: rgba(0, 195, 255, 120);"
        "  border-top: 1px solid rgba(0, 195, 255, 120);"
        "  border-left: 1px solid rgba(0, 195, 255, 120);"
        "  min-width: 10px;"
        "  min-height: 10px;"
        "  margin-right: 5px;"
        "}"
    );

    // Kontener na logo z elementem narożnym
    QWidget* logoContainer = new QWidget(this);
    QHBoxLayout* logoLayout = new QHBoxLayout(logoContainer);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    logoLayout->setSpacing(5);
    logoLayout->addWidget(cornerElement1);
    logoLayout->addWidget(logoLabel);

    // Centralny kontener na przyciski z justify-between
    QWidget* buttonsContainer = new QWidget(this);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(0);

    // Utworzenie przycisków
    createWavelengthButton = new CyberpunkButton("Generate Wavelength", buttonsContainer);
    joinWavelengthButton = new CyberpunkButton("Merge Wavelength", buttonsContainer);

    // Dodanie przycisków do layoutu z odstępem między nimi (justify-between)
    buttonsLayout->addWidget(createWavelengthButton);
    buttonsLayout->addSpacing(40); // Odstęp między przyciskami
    buttonsLayout->addWidget(joinWavelengthButton);

    // Element narożny po prawej
    QLabel* cornerElement2 = new QLabel(this);
    cornerElement2->setStyleSheet(
        "QLabel {"
        "  color: rgba(0, 195, 255, 120);"
        "  border-top: 1px solid rgba(0, 195, 255, 120);"
        "  border-right: 1px solid rgba(0, 195, 255, 120);"
        "  min-width: 10px;"
        "  min-height: 10px;"
        "  margin-left: 5px;"
        "}"
    );

    // Dodanie wszystkich elementów do głównego layoutu
    mainLayout->addWidget(logoContainer, 0, Qt::AlignLeft); // Logo przyciągnięte do lewej
    mainLayout->addStretch(1); // Elastyczna przestrzeń przed kontenerem z przyciskami
    mainLayout->addWidget(buttonsContainer, 0, Qt::AlignCenter); // Kontener z przyciskami wycentrowany
    mainLayout->addStretch(1); // Elastyczna przestrzeń po kontenerze z przyciskami
    mainLayout->addWidget(cornerElement2, 0, Qt::AlignRight); // Element narożny po prawej stronie

    addWidget(mainContainer);

    connect(createWavelengthButton, &QPushButton::clicked, this, &Navbar::createWavelengthClicked);
    connect(joinWavelengthButton, &QPushButton::clicked, this, &Navbar::joinWavelengthClicked);
}

void Navbar::contextMenuEvent(QContextMenuEvent *event) {
    event->ignore();
}