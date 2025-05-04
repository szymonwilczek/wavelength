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
      m_config(WavelengthConfig::GetInstance()),
      // Inicjalizacja wskaźników
      bg_color_preview_(nullptr), blob_color_preview_(nullptr), stream_color_preview_(nullptr),
      grid_color_preview_(nullptr), grid_spacing_spin_box_(nullptr), // NOWE
      title_text_color_preview_(nullptr), title_border_color_preview_(nullptr), title_glow_color_preview_(nullptr), // NOWE
      recent_colors_layout_(nullptr)
{
    SetupUi();
    LoadSettings();
    connect(m_config, &WavelengthConfig::recentColorsChanged, this, &AppearanceSettingsWidget::UpdateRecentColorsUI);
}

void AppearanceSettingsWidget::SetupUi() {
    const auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 20, 20, 20);
    main_layout->setSpacing(20);

    const auto title_label = new QLabel("Appearance Customization", this);
    title_label->setStyleSheet("font-size: 14pt; font-weight: bold; color: #00ccff; background-color: transparent; border: none;");
    main_layout->addWidget(title_label);

    const auto info_label = new QLabel("Configure application appearance colors (click preview to change).", this);
    info_label->setStyleSheet("color: #ffcc00; background-color: transparent; border: none; font-size: 9pt;");
    info_label->setWordWrap(true);
    main_layout->addWidget(info_label);

    const auto grid_layout = new QGridLayout();
    grid_layout->setHorizontalSpacing(15);
    grid_layout->setVerticalSpacing(10);
    grid_layout->setColumnStretch(1, 0); // Podgląd/Input ma stały rozmiar
    grid_layout->setColumnStretch(2, 1); // Pusta kolumna się rozciąga

    const QString label_style = "color: #c0c0c0; background-color: transparent; font-family: Consolas; font-size: 10pt;";
    const QString box_style = "QSpinBox { background-color: #1A2A3A; color: #E0E0E0; border: 1px solid #005577; border-radius: 3px; padding: 3px; font-family: Consolas; }"
                           "QSpinBox::up-button, QSpinBox::down-button { width: 16px; background-color: #005577; border: 1px solid #0077AA; }"
                           "QSpinBox::up-arrow, QSpinBox::down-arrow { color: #E0E0E0; }"; // Prosty styl dla SpinBox

    int row = 0;

    // --- Istniejące kolory ---
    // Background Color
    const auto background_label = new QLabel("Background Color:", this); background_label->setStyleSheet(label_style);
    bg_color_preview_ = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(bg_color_preview_), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::ChooseBackgroundColor);
    grid_layout->addWidget(background_label, row, 0, Qt::AlignRight); grid_layout->addWidget(bg_color_preview_, row, 1); row++;
    // Blob Color
    const auto blob_label = new QLabel("Blob Color:", this); blob_label->setStyleSheet(label_style);
    blob_color_preview_ = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(blob_color_preview_), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::ChooseBlobColor);
    grid_layout->addWidget(blob_label, row, 0, Qt::AlignRight); grid_layout->addWidget(blob_color_preview_, row, 1); row++;
    // Stream Color
    const auto stream_label = new QLabel("Stream Color:", this); stream_label->setStyleSheet(label_style);
    stream_color_preview_ = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(stream_color_preview_), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::ChooseStreamColor);
    grid_layout->addWidget(stream_label, row, 0, Qt::AlignRight); grid_layout->addWidget(stream_color_preview_, row, 1); row++;

    // --- NOWE: Ustawienia siatki ---
    // Grid Color
    const auto grid_color_label = new QLabel("Grid Color:", this); grid_color_label->setStyleSheet(label_style);
    grid_color_preview_ = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(grid_color_preview_), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::ChooseGridColor);
    grid_layout->addWidget(grid_color_label, row, 0, Qt::AlignRight); grid_layout->addWidget(grid_color_preview_, row, 1); row++;
    // Grid Spacing
    const auto grid_spacing_label = new QLabel("Grid Spacing:", this); grid_spacing_label->setStyleSheet(label_style);
    grid_spacing_spin_box_ = new QSpinBox(this);
    grid_spacing_spin_box_->setRange(5, 200); // Ustaw sensowny zakres
    grid_spacing_spin_box_->setSuffix(" px");
    grid_spacing_spin_box_->setStyleSheet(box_style);
    connect(grid_spacing_spin_box_, qOverload<int>(&QSpinBox::valueChanged), this, &AppearanceSettingsWidget::GridSpacingChanged);
    grid_layout->addWidget(grid_spacing_label, row, 0, Qt::AlignRight); grid_layout->addWidget(grid_spacing_spin_box_, row, 1); row++;
    // -----------------------------

    // --- NOWE: Ustawienia tytułu ---
    // Title Text Color
    const auto title_text_label = new QLabel("Title Text Color:", this); title_text_label->setStyleSheet(label_style);
    title_text_color_preview_ = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(title_text_color_preview_), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::ChooseTitleTextColor);
    grid_layout->addWidget(title_text_label, row, 0, Qt::AlignRight); grid_layout->addWidget(title_text_color_preview_, row, 1); row++;
    // Title Border Color
    const auto title_border_label = new QLabel("Title Border Color:", this); title_border_label->setStyleSheet(label_style);
    title_border_color_preview_ = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(title_border_color_preview_), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::ChooseTitleBorderColor);
    grid_layout->addWidget(title_border_label, row, 0, Qt::AlignRight); grid_layout->addWidget(title_border_color_preview_, row, 1); row++;
    // Title Glow Color
    const auto title_glow_label = new QLabel("Title Glow Color:", this); title_glow_label->setStyleSheet(label_style);
    title_glow_color_preview_ = new ClickableColorPreview(this);
    connect(qobject_cast<ClickableColorPreview*>(title_glow_color_preview_), &ClickableColorPreview::clicked, this, &AppearanceSettingsWidget::ChooseTitleGlowColor);
    grid_layout->addWidget(title_glow_label, row, 0, Qt::AlignRight); grid_layout->addWidget(title_glow_color_preview_, row, 1);
    // -----------------------------

    main_layout->addLayout(grid_layout);

    // Sekcja ostatnich kolorów (bez zmian)
    const auto recent_label = new QLabel("Recently Used Colors:", this);
    recent_label->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt; margin-top: 15px;");
    main_layout->addWidget(recent_label);
    const auto recent_colors_container = new QWidget(this);
    recent_colors_layout_ = new QHBoxLayout(recent_colors_container);
    recent_colors_layout_->setContentsMargins(0, 5, 0, 0); recent_colors_layout_->setSpacing(8); recent_colors_layout_->setAlignment(Qt::AlignLeft);
    main_layout->addWidget(recent_colors_container);

    main_layout->addStretch();
}

void AppearanceSettingsWidget::LoadSettings() {
    // Istniejące
    selected_background_color_ = m_config->GetBackgroundColor();
    selected_blob_color_ = m_config->GetBlobColor();
    selected_stream_color_ = m_config->GetStreamColor();
    UpdateColorPreview(bg_color_preview_, selected_background_color_);
    UpdateColorPreview(blob_color_preview_, selected_blob_color_);
    UpdateColorPreview(stream_color_preview_, selected_stream_color_);

    // NOWE
    selected_grid_color_ = m_config->GetGridColor();
    selected_grid_spacing_ = m_config->GetGridSpacing();
    selected_title_text_color_ = m_config->GetTitleTextColor();
    selected_title_border_color_ = m_config->GetTitleBorderColor();
    selected_title_glow_color_ = m_config->GetTitleGlowColor();

    UpdateColorPreview(grid_color_preview_, selected_grid_color_);
    grid_spacing_spin_box_->setValue(selected_grid_spacing_);
    UpdateColorPreview(title_text_color_preview_, selected_title_text_color_);
    UpdateColorPreview(title_border_color_preview_, selected_title_border_color_);
    UpdateColorPreview(title_glow_color_preview_, selected_title_glow_color_);
    // ---

    UpdateRecentColorsUI();
}

void AppearanceSettingsWidget::SaveSettings() const {
    // Wywołaj settery w config - chociaż zmiany są natychmiastowe,
    // to zapewnia spójność, jeśli użytkownik zmienił coś w innych zakładkach
    m_config->SetBackgroundColor(selected_background_color_);
    m_config->SetBlobColor(selected_blob_color_);
    m_config->SetStreamColor(selected_stream_color_);
    // NOWE
    m_config->SetGridColor(selected_grid_color_);
    m_config->SetGridSpacing(selected_grid_spacing_);
    m_config->SetTitleTextColor(selected_title_text_color_);
    m_config->SetTitleBorderColor(selected_title_border_color_);
    m_config->SetTitleGlowColor(selected_title_glow_color_);
    // ---
    // Nie trzeba wywoływać m_config->saveSettings() tutaj, zrobi to główny przycisk Save
}

void AppearanceSettingsWidget::UpdateColorPreview(QWidget* preview_widget, const QColor& color) {
    if (const auto preview = qobject_cast<ClickableColorPreview*>(preview_widget)) {
        preview->SetColor(color);
    } else {
        qWarning() << "Failed to cast QWidget* to ClickableColorPreview* in updateColorPreview.";
    }
}

void AppearanceSettingsWidget::ChooseBackgroundColor() {
    // Zapisz bieżący kolor, aby można było go przywrócić, jeśli użytkownik anuluje
    const QColor original_color = selected_background_color_;
    const QColor color = QColorDialog::getColor(original_color, this, "Select Background Color", QColorDialog::ShowAlphaChannel);

    if (color.isValid()) {
        selected_background_color_ = color; // Zaktualizuj kolor przechowywany lokalnie (na potrzeby przycisku Save)
        UpdateColorPreview(bg_color_preview_, selected_background_color_); // Zaktualizuj lokalny podgląd

        // --- Natychmiast zastosuj zmianę w konfiguracji ---
        // To spowoduje emisję sygnału configChanged("background_color")
        m_config->SetBackgroundColor(color);
        // -------------------------------------------------

        // m_config->addRecentColor(color); // Niepotrzebne, bo setBackgroundColor już to robi
    } else {
        // Użytkownik anulował - przywróć poprzedni kolor w podglądzie (opcjonalnie)
        // updateColorPreview(m_bgColorPreview, originalColor);
        // m_selectedBgColor = originalColor; // Przywróć też zmienną lokalną
    }
}


void AppearanceSettingsWidget::ChooseBlobColor() {
    const QColor original_color = selected_blob_color_;
    const QColor color = QColorDialog::getColor(original_color, this, "Select Blob Color"); // Usunięto ShowAlphaChannel, jeśli niepotrzebne dla bloba

    if (color.isValid()) {
        selected_blob_color_ = color; // Zaktualizuj kolor lokalny
        UpdateColorPreview(blob_color_preview_, selected_blob_color_); // Zaktualizuj podgląd

        // --- Natychmiast zastosuj zmianę w konfiguracji ---
        m_config->SetBlobColor(color); // To wyemituje configChanged("blob_color")
        // -------------------------------------------------

        // m_config->addRecentColor(color); // Niepotrzebne, setBlobColor to robi
    }
    // Opcjonalnie: przywróć kolor, jeśli anulowano
    // else {
    //     updateColorPreview(m_blobColorPreview, originalColor);
    //     m_selectedBlobColor = originalColor;
    // }
}


void AppearanceSettingsWidget::ChooseStreamColor() {
    const QColor original_color = selected_stream_color_;
    const QColor color = QColorDialog::getColor(original_color, this, "Select Stream Color");
    if (color.isValid()) {
        selected_stream_color_ = color;
        UpdateColorPreview(stream_color_preview_, selected_stream_color_);
        // --- Natychmiast zastosuj zmianę (jeśli chcesz) ---
        m_config->SetStreamColor(color); // Zakładając, że chcesz natychmiastowej zmiany
        // -------------------------------------------------
    }
}

void AppearanceSettingsWidget::ChooseGridColor() {
    const QColor original_color = selected_grid_color_;
    const QColor color = QColorDialog::getColor(original_color, this, "Select Grid Color", QColorDialog::ShowAlphaChannel); // Pozwól na alfę dla siatki
    if (color.isValid()) {
        selected_grid_color_ = color;
        UpdateColorPreview(grid_color_preview_, selected_grid_color_);
        m_config->SetGridColor(color);
    }
}

void AppearanceSettingsWidget::GridSpacingChanged(const int value) {
    if (selected_grid_spacing_ != value) {
        selected_grid_spacing_ = value;
        m_config->SetGridSpacing(value);
        // Nie ma podglądu do aktualizacji, SpinBox sam się aktualizuje
    }
}

void AppearanceSettingsWidget::ChooseTitleTextColor() {
    const QColor original_color = selected_title_text_color_;
    const QColor color = QColorDialog::getColor(original_color, this, "Select Title Text Color");
    if (color.isValid()) {
        selected_title_text_color_ = color;
        UpdateColorPreview(title_text_color_preview_, selected_title_text_color_);
        m_config->SetTitleTextColor(color);
    }
}

void AppearanceSettingsWidget::ChooseTitleBorderColor() {
    const QColor original_color = selected_title_border_color_;
    const QColor color = QColorDialog::getColor(original_color, this, "Select Title Border Color");
    if (color.isValid()) {
        selected_title_border_color_ = color;
        UpdateColorPreview(title_border_color_preview_, selected_title_border_color_);
        m_config->SetTitleBorderColor(color);
    }
}

void AppearanceSettingsWidget::ChooseTitleGlowColor() {
    const QColor original_color = selected_title_glow_color_;
    const QColor color = QColorDialog::getColor(original_color, this, "Select Title Glow Color");
    if (color.isValid()) {
        selected_title_glow_color_ = color;
        UpdateColorPreview(title_glow_color_preview_, selected_title_glow_color_);
        m_config->SetTitleGlowColor(color);
    }
}
// --------------------

void AppearanceSettingsWidget::SelectRecentColor(const QColor& color) const {
    // Tutaj można by dodać logikę, który podgląd ma być zaktualizowany
    // na podstawie jakiegoś stanu, ale na razie samo dodanie do listy wystarczy.
    m_config->AddRecentColor(color);
}

void AppearanceSettingsWidget::UpdateRecentColorsUI() {
    if (!recent_colors_layout_) return;

    // Wyczyść stary layout
    qDeleteAll(recent_color_buttons_);
    recent_color_buttons_.clear();
    QLayoutItem* item;
    while ((item = recent_colors_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
             item->widget()->deleteLater();
        }
        // Usuń również element layoutu, jeśli nie jest to stretch
        if (!item->spacerItem()) {
             delete item;
        }
    }
     // Usuń ewentualny stary stretch na końcu
    if (recent_colors_layout_->count() > 0 && recent_colors_layout_->itemAt(recent_colors_layout_->count() - 1)->spacerItem()) {
        item = recent_colors_layout_->takeAt(recent_colors_layout_->count() - 1);
        delete item;
    }


    QStringList recent_hex_colors = m_config->GetRecentColors();

    for (const QString& hex_color : recent_hex_colors) {
        QColor color(hex_color);
        if (color.isValid()) {
            auto color_button = new QPushButton(recent_colors_layout_->parentWidget());
            color_button->setFixedSize(28, 28);
            color_button->setToolTip(hex_color);

            color_button->setStyleSheet(QString(
                "QPushButton {"
                "  background-color: %1;" // Ustaw kolor tła
                "  border: 1px solid #777;"
                "  border-radius: 3px;"
                "}"
                "QPushButton:hover {"
                "  border: 1px solid #ccc;" // Można dodać efekt hover, np. jaśniejszą ramkę
                "}"
            ).arg(color.name(QColor::HexRgb))); // Użyj formatu #RRGGBB


            connect(color_button, &QPushButton::clicked, this, [this, color]() {
                this->SelectRecentColor(color);
            });

            recent_colors_layout_->addWidget(color_button);
            recent_color_buttons_.append(color_button);
        }
    }
    recent_colors_layout_->addStretch(); // Dodaj rozciągliwość na końcu
}