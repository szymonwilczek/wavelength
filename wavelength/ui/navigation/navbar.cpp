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

#include "network_status_widget.h"
#include "../../../font_manager.h"

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

    // Główny kontener z layoutem horyzontalnym na całą szerokość
    QWidget* mainContainer = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Element narożny po lewej
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

    // Logo z neonowym efektem poświaty
    logoLabel = new QLabel("WAVELENGTH", this);

    // Użyj FontManager do pobrania czcionki BlenderPro
    QFont logoFont = FontManager::instance().getFont(FontFamily::BlenderPro, FontStyle::Bold, 12);
    logoLabel->setFont(logoFont);
    logoLabel->setStyleSheet(
        "QLabel {"
        "  color: #f0f0f0;"
        "  padding: 5px 0px;"
        "}"
    );

    // Dodanie neonowego efektu poświaty
    QGraphicsDropShadowEffect* textGlow = new QGraphicsDropShadowEffect(logoLabel);
    textGlow->setBlurRadius(8);
    textGlow->setOffset(0, 0);
    textGlow->setColor(QColor(0, 195, 255, 150));
    logoLabel->setGraphicsEffect(textGlow);

    // Sekcja logo (lewa strona)
    QWidget* logoContainer = new QWidget(this);
    QHBoxLayout* logoLayout = new QHBoxLayout(logoContainer);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    logoLayout->setSpacing(5);
    logoLayout->addWidget(cornerElement1);
    logoLayout->addWidget(logoLabel);

    // NOWY ELEMENT: Widget statusu sieci (środek)
    NetworkStatusWidget* networkStatus = new NetworkStatusWidget(this);
    networkStatus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Kontener na przyciski (prawa strona)
    QWidget* buttonsContainer = new QWidget(this);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(40); // Stały odstęp między przyciskami

    // Przyciski akcji
    createWavelengthButton = new CyberpunkButton("Generate Wavelength", buttonsContainer);
    joinWavelengthButton = new CyberpunkButton("Merge Wavelength", buttonsContainer);
    buttonsLayout->addWidget(createWavelengthButton);
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
    mainLayout->addWidget(logoContainer, 0, Qt::AlignLeft);      // Logo do lewej
    mainLayout->addStretch(1);                                   // Elastyczna przestrzeń
    mainLayout->addWidget(networkStatus, 0, Qt::AlignCenter);    // Status sieci na środku
    mainLayout->addStretch(1);                                   // Elastyczna przestrzeń
    mainLayout->addWidget(buttonsContainer, 0, Qt::AlignRight);  // Przyciski do prawej
    mainLayout->addWidget(cornerElement2, 0, Qt::AlignRight);    // Element narożny po prawej

    addWidget(mainContainer);

    connect(createWavelengthButton, &QPushButton::clicked, this, &Navbar::createWavelengthClicked);
    connect(joinWavelengthButton, &QPushButton::clicked, this, &Navbar::joinWavelengthClicked);
}

void Navbar::contextMenuEvent(QContextMenuEvent *event) {
    event->ignore();
}