#include "appearance_settings_widget.h"
#include "../../../../app/wavelength_config.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QColorDialog>
#include <QDebug>
#include <QSpinBox>

#include "components/clickable_color_preview.h"

AppearanceSettingsWidget::AppearanceSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_config(WavelengthConfig::getInstance()),
      // Inicjalizacja wskaźników
      m_bgColorPreview(nullptr), m_blobColorPreview(nullptr), m_streamColorPreview(nullptr),
      m_gridColorPreview(nullptr), m_gridSpacingSpinBox(nullptr), // NOWE
      m_titleTextColorPreview(nullptr), m_titleBorderColorPreview(nullptr), m_titleGlowColorPreview(nullptr), // NOWE
      m_recentColorsLayout(nullptr)
{
    qDebug() << "AppearanceSettingsWidget constructor start";
    setupUi();
    loadSettings();
    connect(m_config, &WavelengthConfig::recentColorsChanged, this, &AppearanceSettingsWidget::updateRecentColorsUI);
    qDebug() << "AppearanceSettingsWidget constructor end";
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
    gridLayout->setColumnStretch(1, 0); // Podgląd/Input ma stały rozmiar
    gridLayout->setColumnStretch(2, 1); // Pusta kolumna się rozciąga

    QString labelStyle = "color: #c0c0c0; background-color: transparent; font-family: Consolas; font-size: 10pt;";
    QString spinBoxStyle = "QSpinBox { background-color: #1A2A3A; color: #E0E0E0; border: 1px solid #005577; border-radius: 3px; padding: 3px; font-family: Consolas; }"
                           "QSpinBox::up-button, QSpinBox::down-button { width: 16px; background-color: #005577; border: 1px solid #0077AA; }"
                           "QSpinBox::up-arrow, QSpinBox::down-arrow { color: #E0E0E0; }"; // Prosty styl dla SpinBox

    int row = 0;

    // --- Istniejące kolory ---
    // Background Color
    QLabel* bgLabel = new QLabel("Background Color:", this); bgLabel->setStyleSheet(labelStyle);
    m_bgColorPreview = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(m_bgColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseBackgroundColor);
    gridLayout->addWidget(bgLabel, row, 0, Qt::AlignRight); gridLayout->addWidget(m_bgColorPreview, row, 1); row++;
    // Blob Color
    QLabel* blobLabel = new QLabel("Blob Color:", this); blobLabel->setStyleSheet(labelStyle);
    m_blobColorPreview = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(m_blobColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseBlobColor);
    gridLayout->addWidget(blobLabel, row, 0, Qt::AlignRight); gridLayout->addWidget(m_blobColorPreview, row, 1); row++;
    // Stream Color
    QLabel* streamLabel = new QLabel("Stream Color:", this); streamLabel->setStyleSheet(labelStyle);
    m_streamColorPreview = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(m_streamColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseStreamColor);
    gridLayout->addWidget(streamLabel, row, 0, Qt::AlignRight); gridLayout->addWidget(m_streamColorPreview, row, 1); row++;

    // --- NOWE: Ustawienia siatki ---
    // Grid Color
    QLabel* gridColorLabel = new QLabel("Grid Color:", this); gridColorLabel->setStyleSheet(labelStyle);
    m_gridColorPreview = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(m_gridColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseGridColor);
    gridLayout->addWidget(gridColorLabel, row, 0, Qt::AlignRight); gridLayout->addWidget(m_gridColorPreview, row, 1); row++;
    // Grid Spacing
    QLabel* gridSpacingLabel = new QLabel("Grid Spacing:", this); gridSpacingLabel->setStyleSheet(labelStyle);
    m_gridSpacingSpinBox = new QSpinBox(this);
    m_gridSpacingSpinBox->setRange(5, 200); // Ustaw sensowny zakres
    m_gridSpacingSpinBox->setSuffix(" px");
    m_gridSpacingSpinBox->setStyleSheet(spinBoxStyle);
    connect(m_gridSpacingSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &AppearanceSettingsWidget::gridSpacingChanged);
    gridLayout->addWidget(gridSpacingLabel, row, 0, Qt::AlignRight); gridLayout->addWidget(m_gridSpacingSpinBox, row, 1); row++;
    // -----------------------------

    // --- NOWE: Ustawienia tytułu ---
    // Title Text Color
    QLabel* titleTextLabel = new QLabel("Title Text Color:", this); titleTextLabel->setStyleSheet(labelStyle);
    m_titleTextColorPreview = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(m_titleTextColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseTitleTextColor);
    gridLayout->addWidget(titleTextLabel, row, 0, Qt::AlignRight); gridLayout->addWidget(m_titleTextColorPreview, row, 1); row++;
    // Title Border Color
    QLabel* titleBorderLabel = new QLabel("Title Border Color:", this); titleBorderLabel->setStyleSheet(labelStyle);
    m_titleBorderColorPreview = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(m_titleBorderColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseTitleBorderColor);
    gridLayout->addWidget(titleBorderLabel, row, 0, Qt::AlignRight); gridLayout->addWidget(m_titleBorderColorPreview, row, 1); row++;
    // Title Glow Color
    QLabel* titleGlowLabel = new QLabel("Title Glow Color:", this); titleGlowLabel->setStyleSheet(labelStyle);
    m_titleGlowColorPreview = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(m_titleGlowColorPreview), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::chooseTitleGlowColor);
    gridLayout->addWidget(titleGlowLabel, row, 0, Qt::AlignRight); gridLayout->addWidget(m_titleGlowColorPreview, row, 1); row++;
    // -----------------------------

    mainLayout->addLayout(gridLayout);

    // Sekcja ostatnich kolorów (bez zmian)
    QLabel *recentLabel = new QLabel("Recently Used Colors:", this);
    recentLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt; margin-top: 15px;");
    mainLayout->addWidget(recentLabel);
    QWidget* recentColorsContainer = new QWidget(this);
    m_recentColorsLayout = new QHBoxLayout(recentColorsContainer);
    m_recentColorsLayout->setContentsMargins(0, 5, 0, 0); m_recentColorsLayout->setSpacing(8); m_recentColorsLayout->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(recentColorsContainer);

    mainLayout->addStretch();
    qDebug() << "AppearanceSettingsWidget setupUi end";
}

void AppearanceSettingsWidget::loadSettings() {
    // Istniejące
    m_selectedBgColor = m_config->getBackgroundColor();
    m_selectedBlobColor = m_config->getBlobColor();
    m_selectedStreamColor = m_config->getStreamColor();
    updateColorPreview(m_bgColorPreview, m_selectedBgColor);
    updateColorPreview(m_blobColorPreview, m_selectedBlobColor);
    updateColorPreview(m_streamColorPreview, m_selectedStreamColor);

    // NOWE
    m_selectedGridColor = m_config->getGridColor();
    m_selectedGridSpacing = m_config->getGridSpacing();
    m_selectedTitleTextColor = m_config->getTitleTextColor();
    m_selectedTitleBorderColor = m_config->getTitleBorderColor();
    m_selectedTitleGlowColor = m_config->getTitleGlowColor();

    updateColorPreview(m_gridColorPreview, m_selectedGridColor);
    m_gridSpacingSpinBox->setValue(m_selectedGridSpacing);
    updateColorPreview(m_titleTextColorPreview, m_selectedTitleTextColor);
    updateColorPreview(m_titleBorderColorPreview, m_selectedTitleBorderColor);
    updateColorPreview(m_titleGlowColorPreview, m_selectedTitleGlowColor);
    // ---

    updateRecentColorsUI();
}

void AppearanceSettingsWidget::saveSettings() {
    // Wywołaj settery w config - chociaż zmiany są natychmiastowe,
    // to zapewnia spójność, jeśli użytkownik zmienił coś w innych zakładkach
    m_config->setBackgroundColor(m_selectedBgColor);
    m_config->setBlobColor(m_selectedBlobColor);
    m_config->setStreamColor(m_selectedStreamColor);
    // NOWE
    m_config->setGridColor(m_selectedGridColor);
    m_config->setGridSpacing(m_selectedGridSpacing);
    m_config->setTitleTextColor(m_selectedTitleTextColor);
    m_config->setTitleBorderColor(m_selectedTitleBorderColor);
    m_config->setTitleGlowColor(m_selectedTitleGlowColor);
    // ---
    // Nie trzeba wywoływać m_config->saveSettings() tutaj, zrobi to główny przycisk Save
}

void AppearanceSettingsWidget::updateColorPreview(QWidget* previewWidget, const QColor& color) {
    ClickableColorPreview* preview = qobject_cast<ClickableColorPreview*>(previewWidget);
    if (preview) {
        preview->setColor(color);
    } else {
        qWarning() << "Failed to cast QWidget* to ClickableColorPreview* in updateColorPreview.";
    }
}

void AppearanceSettingsWidget::chooseBackgroundColor() {
    // Zapisz bieżący kolor, aby można było go przywrócić, jeśli użytkownik anuluje
    QColor originalColor = m_selectedBgColor;
    QColor color = QColorDialog::getColor(originalColor, this, "Select Background Color", QColorDialog::ShowAlphaChannel);

    if (color.isValid()) {
        m_selectedBgColor = color; // Zaktualizuj kolor przechowywany lokalnie (na potrzeby przycisku Save)
        updateColorPreview(m_bgColorPreview, m_selectedBgColor); // Zaktualizuj lokalny podgląd

        // --- Natychmiast zastosuj zmianę w konfiguracji ---
        // To spowoduje emisję sygnału configChanged("background_color")
        qDebug() << "Immediately setting background color in config to:" << color.name();
        m_config->setBackgroundColor(color);
        // -------------------------------------------------

        // m_config->addRecentColor(color); // Niepotrzebne, bo setBackgroundColor już to robi
    } else {
        // Użytkownik anulował - przywróć poprzedni kolor w podglądzie (opcjonalnie)
        // updateColorPreview(m_bgColorPreview, originalColor);
        // m_selectedBgColor = originalColor; // Przywróć też zmienną lokalną
    }
}


void AppearanceSettingsWidget::chooseBlobColor() {
    QColor originalColor = m_selectedBlobColor;
    QColor color = QColorDialog::getColor(originalColor, this, "Select Blob Color"); // Usunięto ShowAlphaChannel, jeśli niepotrzebne dla bloba

    if (color.isValid()) {
        m_selectedBlobColor = color; // Zaktualizuj kolor lokalny
        updateColorPreview(m_blobColorPreview, m_selectedBlobColor); // Zaktualizuj podgląd

        // --- Natychmiast zastosuj zmianę w konfiguracji ---
        qDebug() << "Immediately setting blob color in config to:" << color.name();
        m_config->setBlobColor(color); // To wyemituje configChanged("blob_color")
        // -------------------------------------------------

        // m_config->addRecentColor(color); // Niepotrzebne, setBlobColor to robi
    }
    // Opcjonalnie: przywróć kolor, jeśli anulowano
    // else {
    //     updateColorPreview(m_blobColorPreview, originalColor);
    //     m_selectedBlobColor = originalColor;
    // }
}


void AppearanceSettingsWidget::chooseStreamColor() {
    QColor originalColor = m_selectedStreamColor;
    QColor color = QColorDialog::getColor(originalColor, this, "Select Stream Color");
    if (color.isValid()) {
        m_selectedStreamColor = color;
        updateColorPreview(m_streamColorPreview, m_selectedStreamColor);
        // --- Natychmiast zastosuj zmianę (jeśli chcesz) ---
        qDebug() << "Immediately setting stream color in config to:" << color.name();
        m_config->setStreamColor(color); // Zakładając, że chcesz natychmiastowej zmiany
        // -------------------------------------------------
    }
}

void AppearanceSettingsWidget::chooseGridColor() {
    QColor originalColor = m_selectedGridColor;
    QColor color = QColorDialog::getColor(originalColor, this, "Select Grid Color", QColorDialog::ShowAlphaChannel); // Pozwól na alfę dla siatki
    if (color.isValid()) {
        m_selectedGridColor = color;
        updateColorPreview(m_gridColorPreview, m_selectedGridColor);
        qDebug() << "Immediately setting grid color in config to:" << color.name(QColor::HexArgb);
        m_config->setGridColor(color);
    }
}

void AppearanceSettingsWidget::gridSpacingChanged(int value) {
    if (m_selectedGridSpacing != value) {
        m_selectedGridSpacing = value;
        qDebug() << "Immediately setting grid spacing in config to:" << value;
        m_config->setGridSpacing(value);
        // Nie ma podglądu do aktualizacji, SpinBox sam się aktualizuje
    }
}

void AppearanceSettingsWidget::chooseTitleTextColor() {
    QColor originalColor = m_selectedTitleTextColor;
    QColor color = QColorDialog::getColor(originalColor, this, "Select Title Text Color");
    if (color.isValid()) {
        m_selectedTitleTextColor = color;
        updateColorPreview(m_titleTextColorPreview, m_selectedTitleTextColor);
        qDebug() << "Immediately setting title text color in config to:" << color.name();
        m_config->setTitleTextColor(color);
    }
}

void AppearanceSettingsWidget::chooseTitleBorderColor() {
    QColor originalColor = m_selectedTitleBorderColor;
    QColor color = QColorDialog::getColor(originalColor, this, "Select Title Border Color");
    if (color.isValid()) {
        m_selectedTitleBorderColor = color;
        updateColorPreview(m_titleBorderColorPreview, m_selectedTitleBorderColor);
        qDebug() << "Immediately setting title border color in config to:" << color.name();
        m_config->setTitleBorderColor(color);
    }
}

void AppearanceSettingsWidget::chooseTitleGlowColor() {
    QColor originalColor = m_selectedTitleGlowColor;
    QColor color = QColorDialog::getColor(originalColor, this, "Select Title Glow Color");
    if (color.isValid()) {
        m_selectedTitleGlowColor = color;
        updateColorPreview(m_titleGlowColorPreview, m_selectedTitleGlowColor);
        qDebug() << "Immediately setting title glow color in config to:" << color.name();
        m_config->setTitleGlowColor(color);
    }
}
// --------------------

void AppearanceSettingsWidget::selectRecentColor(const QColor& color) {
    qDebug() << "Recent color selected:" << color.name();
    // Tutaj można by dodać logikę, który podgląd ma być zaktualizowany
    // na podstawie jakiegoś stanu, ale na razie samo dodanie do listy wystarczy.
    m_config->addRecentColor(color);
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