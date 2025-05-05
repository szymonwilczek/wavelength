#include "navbar.h"
#include "../../ui/buttons/navbar_button.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QSoundEffect>

#include "network_status_widget.h"
#include "../../app/managers/font_manager.h"
#include "../../app/managers/translation_manager.h"

Navbar::Navbar(QWidget *parent) : QToolBar(parent) {
    setMovable(false);
    setFloatable(false);
    setAllowedAreas(Qt::TopToolBarArea);
    setContextMenuPolicy(Qt::PreventContextMenu);
    const TranslationManager* translator = TranslationManager::GetInstance();

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
    const auto main_container = new QWidget(this);
    main_layout_ = new QHBoxLayout(main_container); // Przypisz do m_mainLayout
    main_layout_->setContentsMargins(0, 5, 0, 5);
    main_layout_->setSpacing(0);

    // Element narożny po lewej (bez zmian)
    const auto corner_element1 = new QLabel(this);
    corner_element1->setStyleSheet(
    "QLabel {"
    "  margin-right: 5px;"
    "}"
    );

    // Logo (bez zmian)
    logo_label_ = new QLabel("WAVELENGTH", this);
    logo_label_->setStyleSheet(
    "QLabel {"
   "   font-family: 'Blender Pro Heavy';"
   "   font-size: 28px;"
   "   letter-spacing: 2px;"
   "   color: #ffffff;"
   "   background-color: transparent;"
   "   text-transform: uppercase;"
   "}"
    );
    const auto text_glow = new QGraphicsDropShadowEffect(logo_label_);
    text_glow->setBlurRadius(8);
    text_glow->setOffset(0, 0);
    text_glow->setColor(QColor(0, 195, 255, 150));
    logo_label_->setGraphicsEffect(text_glow);

    // Sekcja logo (lewa strona)
    logo_container_ = new QWidget(this); // Przypisz do m_logoContainer
    const auto logo_layout = new QHBoxLayout(logo_container_);
    logo_layout->setContentsMargins(0, 0, 0, 0);
    logo_layout->setSpacing(5);
    logo_layout->addWidget(corner_element1);
    logo_layout->addWidget(logo_label_);

    // Widget statusu sieci (środek)
    network_status_ = new NetworkStatusWidget(this); // Przypisz do m_networkStatus
    network_status_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Kontener na przyciski (prawa strona)
    buttons_container_ = new QWidget(this); // Przypisz do m_buttonsContainer
    const auto buttons_layout = new QHBoxLayout(buttons_container_);
    buttons_layout->setContentsMargins(0, 0, 0, 0);
    buttons_layout->setSpacing(40);

    // Przyciski akcji (bez zmian)
    create_button_ = new CyberpunkButton(translator->Translate("Navbar.GenerateWavelength", "Generate Wavelength"), buttons_container_);
    join_button_ = new CyberpunkButton(translator->Translate("Navbar.MergeWavelength", "Merge Wavelength"), buttons_container_);
    settings_button_ = new CyberpunkButton(translator->Translate("Navbar.Settings", "Settings"), buttons_container_);

    buttons_layout->addWidget(create_button_);
    buttons_layout->addWidget(join_button_);
    buttons_layout->addWidget(settings_button_);

    // Element narożny po prawej
    corner_element_ = new QLabel(this); // Przypisz do m_cornerElement2
    corner_element_->setStyleSheet(
        "QLabel {"
        "  margin-left: 5px;"
        "}"
    );

    // Utwórz QSpacerItem zamiast addStretch
    spacer1_ = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
    spacer2_ = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

    // Dodanie wszystkich elementów do głównego layoutu
    main_layout_->addWidget(logo_container_, 0, Qt::AlignLeft); // Indeks 0
    main_layout_->addSpacerItem(spacer1_);                     // Indeks 1
    main_layout_->addWidget(network_status_, 0, Qt::AlignCenter); // Indeks 2
    main_layout_->addSpacerItem(spacer2_);                     // Indeks 3
    main_layout_->addWidget(buttons_container_, 0, Qt::AlignRight); // Indeks 4
    main_layout_->addWidget(corner_element_, 0, Qt::AlignRight);   // Indeks 5

    // Ustawienie początkowych współczynników rozciągania dla spacerów
    main_layout_->setStretch(1, 1); // spacer1
    main_layout_->setStretch(3, 1); // spacer2

    addWidget(main_container);

    // Inicjalizacja QSoundEffect (bez zmian)
    click_sound_ = new QSoundEffect(this);
    click_sound_->setSource(QUrl("qrc:/resources/sounds/interface/button_click.wav"));
    click_sound_->setVolume(0.8);

    // Połączenia sygnałów (bez zmian)
    connect(create_button_, &QPushButton::clicked, this, &Navbar::createWavelengthClicked);
    connect(join_button_, &QPushButton::clicked, this, &Navbar::joinWavelengthClicked);
    connect(settings_button_, &QPushButton::clicked, this, &Navbar::settingsClicked);

    // Połącz wszystkie przyciski z odtwarzaniem dźwięku
    connect(create_button_, &QPushButton::clicked, this, &Navbar::PlayClickSound);
    connect(join_button_, &QPushButton::clicked, this, &Navbar::PlayClickSound);
    connect(settings_button_, &QPushButton::clicked, this, &Navbar::PlayClickSound);
}

void Navbar::SetChatMode(const bool inChat) const {
    // Ukryj/pokaż kontener przycisków i prawy narożnik
    buttons_container_->setVisible(!inChat);
    corner_element_->setVisible(!inChat);

    if (inChat) {
        // Tryb czatu: wyłącz rozciąganie drugiego odstępu, aby status sieci przesunął się w prawo
        main_layout_->setStretch(3, 0); // Indeks spacer2
    } else {
        // Tryb normalny: przywróć rozciąganie drugiego odstępu
        main_layout_->setStretch(3, 1); // Indeks spacer2
    }
    // Nie ma potrzeby wywoływania invalidate() lub activate(), setStretch powinien wystarczyć
}


void Navbar::contextMenuEvent(QContextMenuEvent *event) {
    event->ignore(); // Bez zmian
}

void Navbar::PlayClickSound() const {
    if (click_sound_ && click_sound_->isLoaded()) {
        click_sound_->play();
    } else if (click_sound_) {
        qWarning() << "Click sound not loaded:" << click_sound_->source();
    }
}
