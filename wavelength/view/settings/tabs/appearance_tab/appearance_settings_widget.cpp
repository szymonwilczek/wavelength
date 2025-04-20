#include "appearance_settings_widget.h"
#include "../../../../util/wavelength_config.h" // Poprawna ścieżka do config

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QColorDialog>
#include <QDebug>

#include "components/clickable_color_preview.h"

AppearanceSettingsWidget::AppearanceSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_config(WavelengthConfig::getInstance()),
      m_bgColorPreview(nullptr),
      m_blobColorPreview(nullptr),
      m_messageColorPreview(nullptr),
      m_streamColorPreview(nullptr),
      m_recentColorsLayout(nullptr)
{
    qDebug() << "AppearanceSettingsWidget constructor start";
    setupUi();
    loadSettings();
    connect(m_config, &WavelengthConfig::recentColorsChanged, this, &AppearanceSettingsWidget::updateRecentColorsUI);
    qDebug() << "AppearanceSettingsWidget constructor end";

    // --- DODAJ TEST GEOMETRII PO SETUP ---
    if (m_bgColorPreview) {
        qDebug() << "m_bgColorPreview geometry after setupUi:" << m_bgColorPreview->geometry();
        qDebug() << "m_bgColorPreview isVisible:" << m_bgColorPreview->isVisible();
    } else {
        qDebug() << "m_bgColorPreview is NULL after setupUi!";
    }
    // -------------------------------------
}

void AppearanceSettingsWidget::setupUi() {
    qDebug() << "AppearanceSettingsWidget setupUi start";
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    QLabel *infoLabel = new QLabel("Configure application appearance colors (click preview to change)", this);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    mainLayout->addWidget(infoLabel);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(15);
    gridLayout->setVerticalSpacing(10);
    gridLayout->setColumnStretch(1, 0); // Podgląd ma stały rozmiar
    gridLayout->setColumnStretch(2, 1); // Pusta kolumna się rozciąga

    QString labelStyle = "color: #c0c0c0; background-color: transparent; font-family: Consolas; font-size: 10pt;";

    // Wiersz dla koloru tła
    QLabel* bgLabel = new QLabel("Background Color:", this);
    bgLabel->setStyleSheet(labelStyle);
    m_bgColorPreview = new ClickableColorPreview(this);
    qDebug() << "Created m_bgColorPreview:" << m_bgColorPreview; // Czy wskaźnik jest poprawny?
    connect(qobject_cast<ClickableColorPreview*>(m_bgColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseBackgroundColor);
    gridLayout->addWidget(bgLabel, 0, 0, Qt::AlignRight);
    gridLayout->addWidget(m_bgColorPreview, 0, 1);
    qDebug() << "Added m_bgColorPreview to gridLayout at (0, 1)";

    // Wiersz dla koloru bloba
    QLabel* blobLabel = new QLabel("Blob Color:", this);
    blobLabel->setStyleSheet(labelStyle);
    m_blobColorPreview = new ClickableColorPreview(this);
    qDebug() << "Created m_blobColorPreview:" << m_blobColorPreview;
    connect(qobject_cast<ClickableColorPreview*>(m_blobColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseBlobColor);
    gridLayout->addWidget(blobLabel, 1, 0, Qt::AlignRight);
    gridLayout->addWidget(m_blobColorPreview, 1, 1);
    qDebug() << "Added m_blobColorPreview to gridLayout at (1, 1)";

    // Wiersz dla koloru wiadomości
    QLabel* msgLabel = new QLabel("Message Color:", this);
    msgLabel->setStyleSheet(labelStyle);
    m_messageColorPreview = new ClickableColorPreview(this);
    qDebug() << "Created m_messageColorPreview:" << m_messageColorPreview;
    connect(qobject_cast<ClickableColorPreview*>(m_messageColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseMessageColor);
    gridLayout->addWidget(msgLabel, 2, 0, Qt::AlignRight);
    gridLayout->addWidget(m_messageColorPreview, 2, 1);
    qDebug() << "Added m_messageColorPreview to gridLayout at (2, 1)";

    // Wiersz dla koloru strumienia
    QLabel* streamLabel = new QLabel("Stream Color:", this);
    streamLabel->setStyleSheet(labelStyle);
    m_streamColorPreview = new ClickableColorPreview(this);
    qDebug() << "Created m_streamColorPreview:" << m_streamColorPreview;
    connect(qobject_cast<ClickableColorPreview*>(m_streamColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseStreamColor);
    gridLayout->addWidget(streamLabel, 3, 0, Qt::AlignRight);
    gridLayout->addWidget(m_streamColorPreview, 3, 1);
    qDebug() << "Added m_streamColorPreview to gridLayout at (3, 1)";

    mainLayout->addLayout(gridLayout);

    // Sekcja ostatnich kolorów (bez zmian w logice dodawania)
    QLabel *recentLabel = new QLabel("Recently Used Colors:", this);
    recentLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt; margin-top: 15px;");
    mainLayout->addWidget(recentLabel);
    QWidget* recentColorsContainer = new QWidget(this);
    m_recentColorsLayout = new QHBoxLayout(recentColorsContainer);
    m_recentColorsLayout->setContentsMargins(0, 5, 0, 0);
    m_recentColorsLayout->setSpacing(8);
    m_recentColorsLayout->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(recentColorsContainer);

    mainLayout->addStretch();
    qDebug() << "AppearanceSettingsWidget setupUi end";
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

    updateRecentColorsUI();
}

void AppearanceSettingsWidget::saveSettings() {
    m_config->setBackgroundColor(m_selectedBgColor);
    m_config->setBlobColor(m_selectedBlobColor);
    m_config->setMessageColor(m_selectedMessageColor);
    m_config->setStreamColor(m_selectedStreamColor);
    // Nie trzeba tu wywoływać saveSettings() z config, zrobi to główny przycisk Save w SettingsView
}

void AppearanceSettingsWidget::updateColorPreview(QWidget* previewWidget, const QColor& color) {
    ClickableColorPreview* preview = qobject_cast<ClickableColorPreview*>(previewWidget);
    if (preview) {
        qDebug() << "Calling setColor on" << preview << "with color" << color.name();
        preview->setColor(color);
    } else {
        qWarning() << "Failed to cast QWidget* to ClickableColorPreview* in updateColorPreview.";
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
             item->widget()->deleteLater();
        }
        // Usuń również element layoutu, jeśli nie jest to stretch
        if (!item->spacerItem()) {
             delete item;
        }
    }
     // Usuń ewentualny stary stretch na końcu
    if (m_recentColorsLayout->count() > 0 && m_recentColorsLayout->itemAt(m_recentColorsLayout->count() - 1)->spacerItem()) {
        item = m_recentColorsLayout->takeAt(m_recentColorsLayout->count() - 1);
        delete item;
    }


    QStringList recentHexColors = m_config->getRecentColors();

    for (const QString& hexColor : recentHexColors) {
        QColor color(hexColor);
        if (color.isValid()) {
            QPushButton* colorButton = new QPushButton(m_recentColorsLayout->parentWidget());
            colorButton->setFixedSize(28, 28);
            // Usuwamy setFlat i setAutoFillBackground
            // colorButton->setFlat(true);
            // colorButton->setAutoFillBackground(true);
            colorButton->setToolTip(hexColor);

            // Ustaw kolor tła i styl za pomocą setStyleSheet
            colorButton->setStyleSheet(QString(
                "QPushButton {"
                "  background-color: %1;" // Ustaw kolor tła
                "  border: 1px solid #777;"
                "  border-radius: 3px;"
                "}"
                "QPushButton:hover {"
                "  border: 1px solid #ccc;" // Można dodać efekt hover, np. jaśniejszą ramkę
                "}"
            ).arg(color.name(QColor::HexRgb))); // Użyj formatu #RRGGBB

            // Usunięto setPalette

            connect(colorButton, &QPushButton::clicked, this, [this, color]() {
                this->selectRecentColor(color);
            });

            m_recentColorsLayout->addWidget(colorButton);
            m_recentColorButtons.append(colorButton);
        }
    }
    m_recentColorsLayout->addStretch(); // Dodaj rozciągliwość na końcu
}