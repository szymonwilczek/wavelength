#include "navbar.h"
#include "../button/cyberpunk_button.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem> // Upewnij się, że jest dołączone
#include <QFont>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QPushButton>
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QSoundEffect> // Dodaj dołączenie

#include "network_status_widget.h"
#include "../../../font_manager.h"

Navbar::Navbar(QWidget *parent) : QToolBar(parent) {
    setMovable(false);
    setFloatable(false);
    setAllowedAreas(Qt::TopToolBarArea);
    setContextMenuPolicy(Qt::PreventContextMenu);

    // Stylizacja (bez zmian)
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

    // Główny kontener i layout
    QWidget* mainContainer = new QWidget(this);
    m_mainLayout = new QHBoxLayout(mainContainer); // Przypisz do m_mainLayout
    m_mainLayout->setContentsMargins(0, 5, 0, 5);
    m_mainLayout->setSpacing(0);

    // Element narożny po lewej (bez zmian)
    QLabel* cornerElement1 = new QLabel(this);
    cornerElement1->setStyleSheet(
    "QLabel {"
    "  margin-right: 5px;"
    "}"
    );

    // Logo (bez zmian)
    logoLabel = new QLabel("WAVELENGTH", this);
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
    QGraphicsDropShadowEffect* textGlow = new QGraphicsDropShadowEffect(logoLabel);
    textGlow->setBlurRadius(8);
    textGlow->setOffset(0, 0);
    textGlow->setColor(QColor(0, 195, 255, 150));
    logoLabel->setGraphicsEffect(textGlow);

    // Sekcja logo (lewa strona)
    m_logoContainer = new QWidget(this); // Przypisz do m_logoContainer
    QHBoxLayout* logoLayout = new QHBoxLayout(m_logoContainer);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    logoLayout->setSpacing(5);
    logoLayout->addWidget(cornerElement1);
    logoLayout->addWidget(logoLabel);

    // Widget statusu sieci (środek)
    m_networkStatus = new NetworkStatusWidget(this); // Przypisz do m_networkStatus
    m_networkStatus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Kontener na przyciski (prawa strona)
    m_buttonsContainer = new QWidget(this); // Przypisz do m_buttonsContainer
    QHBoxLayout* buttonsLayout = new QHBoxLayout(m_buttonsContainer);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(40);

    // Przyciski akcji (bez zmian)
    createWavelengthButton = new CyberpunkButton("Generate Wavelength", m_buttonsContainer);
    joinWavelengthButton = new CyberpunkButton("Merge Wavelength", m_buttonsContainer);
    settingsButton = new CyberpunkButton("Settings", m_buttonsContainer);

    buttonsLayout->addWidget(createWavelengthButton);
    buttonsLayout->addWidget(joinWavelengthButton);
    buttonsLayout->addWidget(settingsButton);

    // Element narożny po prawej
    m_cornerElement2 = new QLabel(this); // Przypisz do m_cornerElement2
    m_cornerElement2->setStyleSheet(
        "QLabel {"
        "  margin-left: 5px;"
        "}"
    );

    // Utwórz QSpacerItem zamiast addStretch
    m_spacer1 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_spacer2 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

    // Dodanie wszystkich elementów do głównego layoutu
    m_mainLayout->addWidget(m_logoContainer, 0, Qt::AlignLeft); // Indeks 0
    m_mainLayout->addSpacerItem(m_spacer1);                     // Indeks 1
    m_mainLayout->addWidget(m_networkStatus, 0, Qt::AlignCenter); // Indeks 2
    m_mainLayout->addSpacerItem(m_spacer2);                     // Indeks 3
    m_mainLayout->addWidget(m_buttonsContainer, 0, Qt::AlignRight); // Indeks 4
    m_mainLayout->addWidget(m_cornerElement2, 0, Qt::AlignRight);   // Indeks 5

    // Ustawienie początkowych współczynników rozciągania dla spacerów
    m_mainLayout->setStretch(1, 1); // spacer1
    m_mainLayout->setStretch(3, 1); // spacer2

    addWidget(mainContainer);

    // Inicjalizacja QSoundEffect (bez zmian)
    m_clickSound = new QSoundEffect(this);
    m_clickSound->setSource(QUrl("qrc:/assets/audio/interface/button_click.wav"));
    m_clickSound->setVolume(0.8);

    // Połączenia sygnałów (bez zmian)
    connect(createWavelengthButton, &QPushButton::clicked, this, &Navbar::createWavelengthClicked);
    connect(joinWavelengthButton, &QPushButton::clicked, this, &Navbar::joinWavelengthClicked);
    connect(settingsButton, &QPushButton::clicked, this, &Navbar::settingsClicked);

    // Połącz wszystkie przyciski z odtwarzaniem dźwięku
    connect(createWavelengthButton, &QPushButton::clicked, this, &Navbar::playClickSound);
    connect(joinWavelengthButton, &QPushButton::clicked, this, &Navbar::playClickSound);
    connect(settingsButton, &QPushButton::clicked, this, &Navbar::playClickSound);
}

void Navbar::setChatMode(const bool inChat) {
    // Ukryj/pokaż kontener przycisków i prawy narożnik
    m_buttonsContainer->setVisible(!inChat);
    m_cornerElement2->setVisible(!inChat);

    if (inChat) {
        // Tryb czatu: wyłącz rozciąganie drugiego odstępu, aby status sieci przesunął się w prawo
        m_mainLayout->setStretch(3, 0); // Indeks spacer2
    } else {
        // Tryb normalny: przywróć rozciąganie drugiego odstępu
        m_mainLayout->setStretch(3, 1); // Indeks spacer2
    }
    // Nie ma potrzeby wywoływania invalidate() lub activate(), setStretch powinien wystarczyć
}


void Navbar::contextMenuEvent(QContextMenuEvent *event) {
    event->ignore(); // Bez zmian
}

void Navbar::playClickSound() {
    if (m_clickSound && m_clickSound->isLoaded()) {
        m_clickSound->play();
    } else if (m_clickSound) {
        qWarning() << "Click sound not loaded:" << m_clickSound->source();
    }
}