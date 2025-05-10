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
#include "../../app/managers/translation_manager.h"

Navbar::Navbar(QWidget *parent) : QToolBar(parent) {
    setMovable(false);
    setFloatable(false);
    setAllowedAreas(Qt::TopToolBarArea);
    setContextMenuPolicy(Qt::PreventContextMenu);
    const TranslationManager *translator = TranslationManager::GetInstance();

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

    const auto main_container = new QWidget(this);
    main_layout_ = new QHBoxLayout(main_container);
    main_layout_->setContentsMargins(0, 5, 0, 5);
    main_layout_->setSpacing(0);

    const auto corner_element1 = new QLabel(this);
    corner_element1->setStyleSheet(
        "QLabel {"
        "  margin-right: 5px;"
        "}"
    );

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

    logo_container_ = new QWidget(this);
    const auto logo_layout = new QHBoxLayout(logo_container_);
    logo_layout->setContentsMargins(0, 0, 0, 0);
    logo_layout->setSpacing(5);
    logo_layout->addWidget(corner_element1);
    logo_layout->addWidget(logo_label_);

    network_status_ = new NetworkStatusWidget(this);
    network_status_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    buttons_container_ = new QWidget(this);
    const auto buttons_layout = new QHBoxLayout(buttons_container_);
    buttons_layout->setContentsMargins(0, 0, 0, 0);
    buttons_layout->setSpacing(40);

    create_button_ = new CyberpunkButton(
        translator->Translate("Navbar.GenerateWavelength", "Generate Wavelength"),
        buttons_container_);
    join_button_ = new CyberpunkButton(
        translator->Translate("Navbar.MergeWavelength", "Merge Wavelength"),
        buttons_container_);
    settings_button_ = new CyberpunkButton(
        translator->Translate("Navbar.Settings", "Settings"),
        buttons_container_);

    buttons_layout->addWidget(create_button_);
    buttons_layout->addWidget(join_button_);
    buttons_layout->addWidget(settings_button_);

    corner_element_ = new QLabel(this);
    corner_element_->setStyleSheet(
        "QLabel {"
        "  margin-left: 5px;"
        "}"
    );

    spacer1_ = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
    spacer2_ = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

    main_layout_->addWidget(logo_container_, 0, Qt::AlignLeft); // index 0
    main_layout_->addSpacerItem(spacer1_);
    main_layout_->addWidget(network_status_, 0, Qt::AlignCenter);
    main_layout_->addSpacerItem(spacer2_);
    main_layout_->addWidget(buttons_container_, 0, Qt::AlignRight);
    main_layout_->addWidget(corner_element_, 0, Qt::AlignRight); // index 5

    main_layout_->setStretch(1, 1); // spacer1
    main_layout_->setStretch(3, 1); // spacer2

    addWidget(main_container);

    click_sound_ = new QSoundEffect(this);
    click_sound_->setSource(QUrl("qrc:/resources/sounds/interface/button_click.wav"));
    click_sound_->setVolume(0.8);

    connect(create_button_, &QPushButton::clicked, this, &Navbar::createWavelengthClicked);
    connect(join_button_, &QPushButton::clicked, this, &Navbar::joinWavelengthClicked);
    connect(settings_button_, &QPushButton::clicked, this, &Navbar::settingsClicked);

    connect(create_button_, &QPushButton::clicked, this, &Navbar::PlayClickSound);
    connect(join_button_, &QPushButton::clicked, this, &Navbar::PlayClickSound);
    connect(settings_button_, &QPushButton::clicked, this, &Navbar::PlayClickSound);
}

void Navbar::SetChatMode(const bool inChat) const {
    buttons_container_->setVisible(!inChat);
    corner_element_->setVisible(!inChat);

    if (inChat) {
        main_layout_->setStretch(3, 0);
    } else {
        main_layout_->setStretch(3, 1);
    }
}


void Navbar::contextMenuEvent(QContextMenuEvent *event) {
    event->ignore();
}

void Navbar::PlayClickSound() const {
    if (click_sound_ && click_sound_->isLoaded()) {
        click_sound_->play();
    } else if (click_sound_) {
        qWarning() << "[NAVBAR] Click sound not loaded:" << click_sound_->source();
    }
}
