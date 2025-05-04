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

namespace FrequencyLimits {
    constexpr double kMinHz = 130.0;
    constexpr double kMaxValuePerUnit = 999.9;
    constexpr double kMaxHz = 999900000.0;
}

WavelengthSettingsWidget::WavelengthSettingsWidget(QWidget *parent)
    : QWidget(parent),
      config_(WavelengthConfig::GetInstance()),
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

    const auto info_label = new QLabel("Configure frequency preferences", this); // Zmieniono opis
    info_label->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(info_label);

    const auto form_layout = new QFormLayout();
    form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form_layout->setSpacing(15);
    form_layout->setRowWrapPolicy(QFormLayout::WrapLongRows);

    // --- Preferred Start Frequency --- (bez zmian)
    const auto preferred_frequency_label = new QLabel("Preferred Start Frequency:", this);
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

    layout->addLayout(form_layout);
    layout->addStretch();
}

void WavelengthSettingsWidget::LoadSettings() const {
    if (!frequency_value_edit_ || !frequency_unit_combo_) { // Usunięto sprawdzenie m_serverAddressEdit, m_serverPortEdit
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
    if (!frequency_value_edit_ || !frequency_unit_combo_) { // Usunięto sprawdzenie m_serverAddressEdit, m_serverPortEdit
        qWarning() << "WavelengthSettingsWidget::saveSettings - UI elements not initialized.";
        return;
    }

    double frequency_value_hz = 0.0;
    if (!ValidateFrequencyInput(frequency_value_hz)) {
        qWarning() << "WavelengthSettingsWidget::saveSettings - Invalid frequency input. Settings not saved.";
        return;
    }

    const QString frequency_string_hz = QString::number(frequency_value_hz, 'f', 1);
    config_->SetPreferredStartFrequency(frequency_string_hz);
}