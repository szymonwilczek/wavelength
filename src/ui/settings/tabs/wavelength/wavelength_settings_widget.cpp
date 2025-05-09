#include "wavelength_settings_widget.h"
#include "../../../../app/wavelength_config.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDebug>
#include <QString>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QLocale>

#include "../../../../app/managers/translation_manager.h"

namespace FrequencyLimits {
    constexpr double kMinHz = 130.0;
    constexpr double kMaxValuePerUnit = 999.9;
    constexpr double kMaxHz = 999900000.0;
}

WavelengthSettingsWidget::WavelengthSettingsWidget(QWidget *parent)
    : QWidget(parent),
      config_(WavelengthConfig::GetInstance()),
      translator_(TranslationManager::GetInstance()),
      language_combo_(nullptr),
      frequency_value_edit_(nullptr),
      frequency_unit_combo_(nullptr)
{
    SetupUi();
    LoadSettings();
}

void WavelengthSettingsWidget::SetupUi() {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    const auto title_label = new QLabel(
        translator_->Translate("WavelengthSettingsWidget.Title", "Wavelength Preferences"),
        this);
    title_label->setStyleSheet("font-size: 14pt; font-weight: bold; color: #00ccff; background-color: transparent; border: none;");
    layout->addWidget(title_label);

    const auto info_label = new QLabel(
        translator_->Translate("WavelengthSettingsWidget.Info", "Configure frequency preferences.\nDo you have some idea for some other settings? Open issue on GitHub!"),
        this);
    info_label->setStyleSheet("color: #ffcc00; background-color: transparent; border: none; font-size: 9pt;");
    info_label->setWordWrap(true);
    layout->addWidget(info_label);

    const auto form_layout = new QFormLayout();
    form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form_layout->setSpacing(15);
    form_layout->setRowWrapPolicy(QFormLayout::WrapLongRows);

    // --- Preferred Start Frequency --- (bez zmian)
    const auto preferred_frequency_label = new QLabel(
        translator_->Translate("WavelengthSettingsWidget.WavelengthFrequency", "Preferred Start Frequency:"),
        this);
    preferred_frequency_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    const auto freq_layout = new QHBoxLayout();
    freq_layout->setSpacing(5);
    frequency_value_edit_ = new QLineEdit(this);
    frequency_value_edit_->setPlaceholderText("e.g., 150.5 or 98.7");
    const auto validator = new QDoubleValidator(0.0, FrequencyLimits::kMaxValuePerUnit * 10, 3, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::C);
    frequency_value_edit_->setValidator(validator);
    frequency_value_edit_->setStyleSheet(
        "QLineEdit { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
    );
    freq_layout->addWidget(frequency_value_edit_, 3);
    frequency_unit_combo_ = new QComboBox(this);
    frequency_unit_combo_->addItems({"Hz", "kHz", "MHz"});
    frequency_unit_combo_->setStyleSheet(
        "QComboBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
        "QComboBox::drop-down { border: none; background-color: rgba(0, 150, 220, 100); }"
        "QComboBox QAbstractItemView { background-color: #0A1928; color: #00eeff; selection-background-color: #4682B4; }"
    );
    freq_layout->addWidget(frequency_unit_combo_, 1);
    form_layout->addRow(preferred_frequency_label, freq_layout);

    const auto language_label = new QLabel(translator_->Translate("WavelengthSettingsWidget.LanguageLabel", "Language:"), this);
    language_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    language_combo_ = new QComboBox(this);
    // Dodaj języki z tłumaczeniami i danymi (kodami języków)
    language_combo_->addItem(translator_->Translate("WavelengthSettingsWidget.LanguageEnglish", "English"), "en");
    language_combo_->addItem(translator_->Translate("WavelengthSettingsWidget.LanguagePolish", "Polski"), "pl");
    language_combo_->setStyleSheet(
        "QComboBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
        "QComboBox::drop-down { border: none; background-color: rgba(0, 150, 220, 100); }"
        "QComboBox QAbstractItemView { background-color: #0A1928; color: #00eeff; selection-background-color: #4682B4; }"
    );
    form_layout->addRow(language_label, language_combo_);

    layout->addLayout(form_layout);
    layout->addStretch();
}

void WavelengthSettingsWidget::LoadSettings() const {
    if (!frequency_value_edit_ || !frequency_unit_combo_ || !language_combo_) { // Dodano sprawdzenie language_combo_
        qWarning() << "WavelengthSettingsWidget::loadSettings - UI elements not initialized.";
        return;
    }

    const QString frequency_string = config_->GetPreferredStartFrequency();
    bool ok;
    double frequency_hz = frequency_string.toDouble(&ok);

    if (!ok || frequency_hz < FrequencyLimits::kMinHz) {
        qWarning() << "WavelengthSettingsWidget::loadSettings - Invalid frequency in config:" << frequency_string << ". Using default 130.0 Hz.";
        frequency_hz = FrequencyLimits::kMinHz;
    }

    double display_value = frequency_hz;
    int unit_index = 0;

    if (frequency_hz >= 1000000.0 && frequency_hz <= FrequencyLimits::kMaxHz) {
        display_value = frequency_hz / 1000000.0;
        unit_index = 2;
    } else if (frequency_hz >= 1000.0) {
        display_value = frequency_hz / 1000.0;
        unit_index = 1;
    }

    frequency_value_edit_->setText(QString::number(display_value, 'f', (unit_index > 0) ? 3 : 1));
    frequency_unit_combo_->setCurrentIndex(unit_index);

    const QString current_lang_code = config_->GetLanguageCode();
    const int lang_index = language_combo_->findData(current_lang_code);
    if (lang_index != -1) {
        language_combo_->setCurrentIndex(lang_index);
    } else {
        qWarning() << "WavelengthSettingsWidget::loadSettings - Saved language code not found in combo box:" << current_lang_code << ". Selecting default.";
        language_combo_->setCurrentIndex(0); // Wybierz pierwszy (domyślnie angielski)
    }
}

bool WavelengthSettingsWidget::ValidateFrequencyInput(double& hz) {
    if (!frequency_value_edit_ || !frequency_unit_combo_) return false;

    const QString value_str = frequency_value_edit_->text();
    const QString unit_str = frequency_unit_combo_->currentText();
    bool ok;

    const QLocale c_locale(QLocale::C);
    const double value_double = c_locale.toDouble(value_str, &ok);

    if (!ok || value_double <= 0) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid positive number for the frequency value.");
        frequency_value_edit_->setFocus();
        frequency_value_edit_->selectAll();
        return false;
    }

    if (value_double > FrequencyLimits::kMaxValuePerUnit) {
         QMessageBox::warning(this, "Invalid Input", QString("The maximum value for %1 is %2.").arg(unit_str).arg(FrequencyLimits::kMaxValuePerUnit));
         frequency_value_edit_->setFocus();
         frequency_value_edit_->selectAll();
         return false;
    }

    if (unit_str == "kHz") {
        hz = value_double * 1000.0;
    } else if (unit_str == "MHz") {
        hz = value_double * 1000000.0;
    } else {
        hz = value_double;
    }

    if (hz < FrequencyLimits::kMinHz) {
        QMessageBox::warning(this, "Invalid Input", QString("The minimum frequency allowed is %1 Hz.").arg(FrequencyLimits::kMinHz));
         frequency_value_edit_->setFocus();
         frequency_value_edit_->selectAll();
        return false;
    }

     if (hz > FrequencyLimits::kMaxHz) {
         QMessageBox::warning(this, "Invalid Input", QString("The maximum frequency allowed is %1 MHz.").arg(FrequencyLimits::kMaxValuePerUnit));
         frequency_value_edit_->setFocus();
         frequency_value_edit_->selectAll();
         return false;
     }

    return true;
}


void WavelengthSettingsWidget::SaveSettings() {
    if (!frequency_value_edit_ || !frequency_unit_combo_ || !language_combo_) { // Dodano sprawdzenie language_combo_
        qWarning() << "WavelengthSettingsWidget::saveSettings - UI elements not initialized.";
        return;
    }

    // --- Zapis częstotliwości (bez zmian) ---
    double frequency_value_hz = 0.0;
    if (!ValidateFrequencyInput(frequency_value_hz)) {
        qWarning() << "WavelengthSettingsWidget::saveSettings - Invalid frequency input. Settings not saved.";
        // Nie zapisuj niczego, jeśli częstotliwość jest niepoprawna
        return;
    }
    const QString frequency_string_hz = QString::number(frequency_value_hz, 'f', 1);
    config_->SetPreferredStartFrequency(frequency_string_hz);


    // --- NOWE: Zapis języka i pokazanie komunikatu o restarcie ---
    const QString previous_language_code = config_->GetLanguageCode();
    const QString selected_language_code = language_combo_->currentData().toString();

    if (previous_language_code != selected_language_code) {
        config_->SetLanguageCode(selected_language_code);
        // Zapisz wszystkie ustawienia (w tym język i częstotliwość)
        config_->SaveSettings();

        // Pokaż komunikat o konieczności restartu
        QMessageBox::information(this,
                                 translator_->Translate("Global.RestartRequiredTitle", "Restart Required"),
                                 translator_->Translate("Global.RestartRequiredMessage", "Language settings have been changed..."));
        qInfo() << "Language setting changed to" << selected_language_code << ". Restart required.";
    } else {
        // Język się nie zmienił, zapisz tylko inne ustawienia (częstotliwość)
        config_->SaveSettings();
    }
}