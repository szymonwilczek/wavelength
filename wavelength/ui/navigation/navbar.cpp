#include "navbar.h"
#include "../button/cyberpunk_button.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QFont>
#include <QContextMenuEvent>
#include <QCoreApplication>
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
    mainLayout->setContentsMargins(0, 5, 0, 5);
    mainLayout->setSpacing(0);

    // Element narożny po lewej
    QLabel* cornerElement1 = new QLabel(this);
    cornerElement1->setStyleSheet(
    "QLabel {"
    "  margin-right: 5px;"
    "}"
    );

    // Logo z neonowym efektem poświaty
    logoLabel = new QLabel("WAVELENGTH", this);

    // Ustaw czcionkę za pomocą stylu CSS
    logoLabel->setStyleSheet(
    "QLabel {"
   "   font-family: 'Blender Pro Heavy';"
   "   font-size: 28px;"
   "   letter-spacing: 2px;"
   "   color: #ffffff;"
   "   background-color: transparent;"
   "   text-transform: uppercase;"
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

    // Widget statusu sieci (środek)
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
    settingsButton = new CyberpunkButton("Settings", buttonsContainer);

    buttonsLayout->addWidget(createWavelengthButton);
    buttonsLayout->addWidget(joinWavelengthButton);
    buttonsLayout->addWidget(settingsButton);

    // Element narożny po prawej
    QLabel* cornerElement2 = new QLabel(this);
    cornerElement2->setStyleSheet(
        "QLabel {"
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

    // Inicjalizacja QSoundEffect
    m_clickSound = new QSoundEffect(this);
    m_clickSound->setSource(QUrl("qrc:/assets/audio/interface/button_click.wav"));
    m_clickSound->setVolume(0.8); // Ustaw głośność (0.0 - 1.0)

    // Połączenia sygnałów przycisków z emisją sygnałów Navbara (bez zmian)
    connect(createWavelengthButton, &QPushButton::clicked, this, &Navbar::createWavelengthClicked);
    connect(joinWavelengthButton, &QPushButton::clicked, this, &Navbar::joinWavelengthClicked);
    connect(settingsButton, &QPushButton::clicked, this, &Navbar::settingsClicked);

    connect(settingsButton, &QPushButton::clicked, this, &Navbar::playClickSound);
}

void Navbar::contextMenuEvent(QContextMenuEvent *event) {
    event->ignore();
}

void Navbar::playClickSound() {
    if (m_clickSound && m_clickSound->isLoaded()) { // Sprawdź, czy dźwięk jest załadowany
        m_clickSound->play();
    } else if (m_clickSound) {
        qWarning() << "Click sound not loaded:" << m_clickSound->source();
    }
}