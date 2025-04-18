#include "appearance_settings_widget.h"
#include "../../../../util/wavelength_config.h" // Poprawna ścieżka do config

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QColorDialog>
#include <QDebug>

AppearanceSettingsWidget::AppearanceSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_config(WavelengthConfig::getInstance()), // Pobierz instancję config
      m_bgColorPreview(nullptr),
      m_blobColorPreview(nullptr),
      m_messageColorPreview(nullptr),
      m_streamColorPreview(nullptr),
      m_recentColorsLayout(nullptr)
{
    setupUi();
    loadSettings(); // Załaduj ustawienia przy tworzeniu widgetu

    // Połącz sygnał zmiany ostatnich kolorów z config z aktualizacją UI w tym widgecie
    connect(m_config, &WavelengthConfig::recentColorsChanged, this, &AppearanceSettingsWidget::updateRecentColorsUI);
}

void AppearanceSettingsWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this); // Główny layout pionowy
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    QLabel *infoLabel = new QLabel("Configure application appearance colors", this);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    mainLayout->addWidget(infoLabel);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(15);
    gridLayout->setVerticalSpacing(10);
    gridLayout->setColumnStretch(2, 1); // Rozciągnij trzecią kolumnę

    // Wiersz dla koloru tła
    QLabel* bgLabel = new QLabel("Background Color:", this);
    m_bgColorPreview = new QWidget(this);
    m_bgColorPreview->setFixedSize(24, 24);
    m_bgColorPreview->setAutoFillBackground(true);
    m_bgColorPreview->setStyleSheet("border: 1px solid #555;");
    QPushButton* bgButton = new QPushButton("Choose...", this);
    connect(bgButton, &QPushButton::clicked, this, &AppearanceSettingsWidget::chooseBackgroundColor);
    gridLayout->addWidget(bgLabel, 0, 0, Qt::AlignRight);
    gridLayout->addWidget(m_bgColorPreview, 0, 1);
    gridLayout->addWidget(bgButton, 0, 2);

    // Wiersz dla koloru bloba
    QLabel* blobLabel = new QLabel("Blob Color:", this);
    m_blobColorPreview = new QWidget(this);
    m_blobColorPreview->setFixedSize(24, 24);
    m_blobColorPreview->setAutoFillBackground(true);
    m_blobColorPreview->setStyleSheet("border: 1px solid #555;");
    QPushButton* blobButton = new QPushButton("Choose...", this);
    connect(blobButton, &QPushButton::clicked, this, &AppearanceSettingsWidget::chooseBlobColor);
    gridLayout->addWidget(blobLabel, 1, 0, Qt::AlignRight);
    gridLayout->addWidget(m_blobColorPreview, 1, 1);
    gridLayout->addWidget(blobButton, 1, 2);

    // Wiersz dla koloru wiadomości
    QLabel* msgLabel = new QLabel("Message Color:", this);
    m_messageColorPreview = new QWidget(this);
    m_messageColorPreview->setFixedSize(24, 24);
    m_messageColorPreview->setAutoFillBackground(true);
    m_messageColorPreview->setStyleSheet("border: 1px solid #555;");
    QPushButton* msgButton = new QPushButton("Choose...", this);
    connect(msgButton, &QPushButton::clicked, this, &AppearanceSettingsWidget::chooseMessageColor);
    gridLayout->addWidget(msgLabel, 2, 0, Qt::AlignRight);
    gridLayout->addWidget(m_messageColorPreview, 2, 1);
    gridLayout->addWidget(msgButton, 2, 2);

    // Wiersz dla koloru strumienia
    QLabel* streamLabel = new QLabel("Stream Color:", this);
    m_streamColorPreview = new QWidget(this);
    m_streamColorPreview->setFixedSize(24, 24);
    m_streamColorPreview->setAutoFillBackground(true);
    m_streamColorPreview->setStyleSheet("border: 1px solid #555;");
    QPushButton* streamButton = new QPushButton("Choose...", this);
    connect(streamButton, &QPushButton::clicked, this, &AppearanceSettingsWidget::chooseStreamColor);
    gridLayout->addWidget(streamLabel, 3, 0, Qt::AlignRight);
    gridLayout->addWidget(m_streamColorPreview, 3, 1);
    gridLayout->addWidget(streamButton, 3, 2);

    // Zastosuj style do QLabel i QPushButton w gridLayout
    QString labelStyle = "color: #c0c0c0; background-color: transparent; font-family: Consolas; font-size: 10pt;";
    QString buttonStyle = "QPushButton { background-color: #005577; border: 1px solid #0077AA; padding: 5px 10px; border-radius: 3px; color: #E0E0E0; font-family: Consolas; }"
                          "QPushButton:hover { background-color: #0077AA; }"
                          "QPushButton:pressed { background-color: #003355; }";

    bgLabel->setStyleSheet(labelStyle);
    blobLabel->setStyleSheet(labelStyle);
    msgLabel->setStyleSheet(labelStyle);
    streamLabel->setStyleSheet(labelStyle);
    bgButton->setStyleSheet(buttonStyle);
    blobButton->setStyleSheet(buttonStyle);
    msgButton->setStyleSheet(buttonStyle);
    streamButton->setStyleSheet(buttonStyle);

    mainLayout->addLayout(gridLayout);

    // Sekcja ostatnich kolorów
    QLabel *recentLabel = new QLabel("Recently Used Colors:", this);
    recentLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt; margin-top: 15px;");
    mainLayout->addWidget(recentLabel);

    QWidget* recentColorsContainer = new QWidget(this);
    m_recentColorsLayout = new QHBoxLayout(recentColorsContainer);
    m_recentColorsLayout->setContentsMargins(0, 5, 0, 0);
    m_recentColorsLayout->setSpacing(8);
    m_recentColorsLayout->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(recentColorsContainer);

    mainLayout->addStretch(); // Dodaj rozciągliwość na dole
}

void AppearanceSettingsWidget::loadSettings() {
    m_selectedBgColor = m_config->getBackgroundColor();
    m_selectedBlobColor = m_config->getBlobColor();
    m_selectedMessageColor = m_config->getMessageColor();
    m_selectedStreamColor = m_config->getStreamColor();

    updateColorPreview(m_bgColorPreview, m_selectedBgColor);
    updateColorPreview(m_blobColorPreview, m_selectedBlobColor);
    updateColorPreview(m_messageColorPreview, m_selectedMessageColor);
    updateColorPreview(m_streamColorPreview, m_selectedStreamColor);

    updateRecentColorsUI(); // Zaktualizuj UI ostatnich kolorów
}

void AppearanceSettingsWidget::saveSettings() {
    m_config->setBackgroundColor(m_selectedBgColor);
    m_config->setBlobColor(m_selectedBlobColor);
    m_config->setMessageColor(m_selectedMessageColor);
    m_config->setStreamColor(m_selectedStreamColor);
    // Nie trzeba tu wywoływać saveSettings() z config, zrobi to główny przycisk Save w SettingsView
}

void AppearanceSettingsWidget::updateColorPreview(QWidget* previewWidget, const QColor& color) {
    if (previewWidget) {
        QPalette pal = previewWidget->palette();
        pal.setColor(QPalette::Window, color);
        previewWidget->setPalette(pal);
    }
}

void AppearanceSettingsWidget::chooseBackgroundColor() {
    QColor color = QColorDialog::getColor(m_selectedBgColor, this, "Select Background Color", QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_selectedBgColor = color;
        updateColorPreview(m_bgColorPreview, m_selectedBgColor);
        m_config->addRecentColor(color); // Dodaj do ostatnich od razu
    }
}

void AppearanceSettingsWidget::chooseBlobColor() {
    QColor color = QColorDialog::getColor(m_selectedBlobColor, this, "Select Blob Color");
    if (color.isValid()) {
        m_selectedBlobColor = color;
        updateColorPreview(m_blobColorPreview, m_selectedBlobColor);
        m_config->addRecentColor(color);
    }
}

void AppearanceSettingsWidget::chooseMessageColor() {
    QColor color = QColorDialog::getColor(m_selectedMessageColor, this, "Select Message Color");
    if (color.isValid()) {
        m_selectedMessageColor = color;
        updateColorPreview(m_messageColorPreview, m_selectedMessageColor);
        m_config->addRecentColor(color);
    }
}

void AppearanceSettingsWidget::chooseStreamColor() {
    QColor color = QColorDialog::getColor(m_selectedStreamColor, this, "Select Stream Color");
    if (color.isValid()) {
        m_selectedStreamColor = color;
        updateColorPreview(m_streamColorPreview, m_selectedStreamColor);
        m_config->addRecentColor(color);
    }
}

void AppearanceSettingsWidget::selectRecentColor(const QColor& color) {
    qDebug() << "Recent color selected:" << color.name();
    m_config->addRecentColor(color); // Dodaj ponownie, aby odświeżyć kolejność
}

void AppearanceSettingsWidget::updateRecentColorsUI() {
    if (!m_recentColorsLayout) return;

    // Wyczyść stary layout
    qDeleteAll(m_recentColorButtons);
    m_recentColorButtons.clear();
    QLayoutItem* item;
    while ((item = m_recentColorsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
             item->widget()->deleteLater(); // Bezpieczniejsze usuwanie widgetów
        }
        delete item;
    }

    QStringList recentHexColors = m_config->getRecentColors();

    for (const QString& hexColor : recentHexColors) {
        QColor color(hexColor);
        if (color.isValid()) {
            QPushButton* colorButton = new QPushButton(m_recentColorsLayout->parentWidget());
            colorButton->setFixedSize(28, 28);
            colorButton->setFlat(true);
            colorButton->setAutoFillBackground(true);
            QPalette pal = colorButton->palette();
            pal.setColor(QPalette::Button, color);
            colorButton->setPalette(pal);
            colorButton->setStyleSheet(QString("QPushButton { border: 1px solid #777; border-radius: 3px; } QPushButton:hover { border: 1px solid #ccc; }"));
            colorButton->setToolTip(hexColor);

            connect(colorButton, &QPushButton::clicked, this, [this, color]() {
                this->selectRecentColor(color);
            });

            m_recentColorsLayout->addWidget(colorButton);
            m_recentColorButtons.append(colorButton);
        }
    }
     // Usuń ewentualny stary stretch
    if (m_recentColorsLayout->count() > recentHexColors.size()) {
        item = m_recentColorsLayout->takeAt(m_recentColorsLayout->count() - 1);
        delete item;
    }
    m_recentColorsLayout->addStretch(); // Dodaj rozciągliwość na końcu
}