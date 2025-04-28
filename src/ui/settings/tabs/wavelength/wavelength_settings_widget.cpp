// filepath: c:\Users\szymo\Documents\GitHub\wavelength\wavelength\view\settings\tabs\wavelength_tab\wavelength_settings_widget.cpp
#include "wavelength_settings_widget.h"
#include "../../../../app/wavelength_config.h"
// Usunięto: #include "../../../../ui/input/cyber_line_edit.h"

#include <QLabel>
// Usunięto: #include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDebug>
#include <QString>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QLocale>

// Definicje stałych dla walidacji
namespace FreqLimits {
    const double MIN_HZ = 130.0;
    const double MAX_VALUE_PER_UNIT = 999.9;
    const double MAX_HZ = 999900000.0;
}

WavelengthSettingsWidget::WavelengthSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_config(WavelengthConfig::getInstance()),
      m_frequencyValueEdit(nullptr),
      m_frequencyUnitCombo(nullptr)
      // Usunięto: m_serverAddressEdit(nullptr),
      // Usunięto: m_serverPortEdit(nullptr)
{
    qDebug() << "WavelengthSettingsWidget constructor start";
    setupUi();
    loadSettings();
    qDebug() << "WavelengthSettingsWidget constructor end";
}

void WavelengthSettingsWidget::setupUi() {
    qDebug() << "WavelengthSettingsWidget setupUi start";
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    QLabel *infoLabel = new QLabel("Configure frequency preferences", this); // Zmieniono opis
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(infoLabel);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(15);
    formLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);

    // --- Preferred Start Frequency --- (bez zmian)
    QLabel *preferredFreqLabel = new QLabel("Preferred Start Frequency:", this);
    preferredFreqLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    QHBoxLayout *freqLayout = new QHBoxLayout();
    freqLayout->setSpacing(5);
    m_frequencyValueEdit = new QLineEdit(this);
    m_frequencyValueEdit->setPlaceholderText("e.g., 150.5 or 98.7");
    QDoubleValidator *validator = new QDoubleValidator(0.0, FreqLimits::MAX_VALUE_PER_UNIT * 10, 3, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::C);
    m_frequencyValueEdit->setValidator(validator);
    m_frequencyValueEdit->setStyleSheet(
        "QLineEdit { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
    );
    freqLayout->addWidget(m_frequencyValueEdit, 3);
    m_frequencyUnitCombo = new QComboBox(this);
    m_frequencyUnitCombo->addItems({"Hz", "kHz", "MHz"});
    m_frequencyUnitCombo->setStyleSheet(
        "QComboBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
        "QComboBox::drop-down { border: none; background-color: rgba(0, 150, 220, 100); }"
        "QComboBox QAbstractItemView { background-color: #0A1928; color: #00eeff; selection-background-color: #4682B4; }"
    );
    freqLayout->addWidget(m_frequencyUnitCombo, 1);
    formLayout->addRow(preferredFreqLabel, freqLayout);

    // --- Usunięto Server Address ---
    // --- Usunięto Server Port ---

    layout->addLayout(formLayout);
    layout->addStretch();
    qDebug() << "WavelengthSettingsWidget setupUi end";
}

void WavelengthSettingsWidget::loadSettings() {
    if (!m_frequencyValueEdit || !m_frequencyUnitCombo) { // Usunięto sprawdzenie m_serverAddressEdit, m_serverPortEdit
        qWarning() << "WavelengthSettingsWidget::loadSettings - UI elements not initialized.";
        return;
    }

    QString freqStringHz = m_config->getPreferredStartFrequency();
    bool ok;
    double freqValueHz = freqStringHz.toDouble(&ok);

    if (!ok || freqValueHz < FreqLimits::MIN_HZ) {
        qWarning() << "WavelengthSettingsWidget::loadSettings - Invalid frequency in config:" << freqStringHz << ". Using default 130.0 Hz.";
        freqValueHz = FreqLimits::MIN_HZ;
    }

    double displayValue = freqValueHz;
    int unitIndex = 0;

    if (freqValueHz >= 1000000.0 && freqValueHz <= FreqLimits::MAX_HZ) {
        displayValue = freqValueHz / 1000000.0;
        unitIndex = 2;
    } else if (freqValueHz >= 1000.0) {
        displayValue = freqValueHz / 1000.0;
        unitIndex = 1;
    }

    m_frequencyValueEdit->setText(QString::number(displayValue, 'f', (unitIndex > 0) ? 3 : 1));
    m_frequencyUnitCombo->setCurrentIndex(unitIndex);

    // Usunięto ładowanie m_serverAddressEdit->setText(...)
    // Usunięto ładowanie m_serverPortEdit->setValue(...)

    qDebug() << "WavelengthSettingsWidget settings loaded. Displayed:" << displayValue << m_frequencyUnitCombo->currentText();
}

bool WavelengthSettingsWidget::validateFrequencyInput(double& valueHz) {
    if (!m_frequencyValueEdit || !m_frequencyUnitCombo) return false;

    QString valueStr = m_frequencyValueEdit->text();
    QString unitStr = m_frequencyUnitCombo->currentText();
    bool ok;

    QLocale cLocale(QLocale::C);
    double valueDouble = cLocale.toDouble(valueStr, &ok);

    if (!ok || valueDouble <= 0) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid positive number for the frequency value.");
        m_frequencyValueEdit->setFocus();
        m_frequencyValueEdit->selectAll();
        return false;
    }

    if (valueDouble > FreqLimits::MAX_VALUE_PER_UNIT) {
         QMessageBox::warning(this, "Invalid Input", QString("The maximum value for %1 is %2.").arg(unitStr).arg(FreqLimits::MAX_VALUE_PER_UNIT));
         m_frequencyValueEdit->setFocus();
         m_frequencyValueEdit->selectAll();
         return false;
    }

    if (unitStr == "kHz") {
        valueHz = valueDouble * 1000.0;
    } else if (unitStr == "MHz") {
        valueHz = valueDouble * 1000000.0;
    } else {
        valueHz = valueDouble;
    }

    if (valueHz < FreqLimits::MIN_HZ) {
        QMessageBox::warning(this, "Invalid Input", QString("The minimum frequency allowed is %1 Hz.").arg(FreqLimits::MIN_HZ));
         m_frequencyValueEdit->setFocus();
         m_frequencyValueEdit->selectAll();
        return false;
    }

     if (valueHz > FreqLimits::MAX_HZ) {
         QMessageBox::warning(this, "Invalid Input", QString("The maximum frequency allowed is %1 MHz.").arg(FreqLimits::MAX_VALUE_PER_UNIT));
         m_frequencyValueEdit->setFocus();
         m_frequencyValueEdit->selectAll();
         return false;
     }

    return true;
}


void WavelengthSettingsWidget::saveSettings() {
    if (!m_frequencyValueEdit || !m_frequencyUnitCombo) { // Usunięto sprawdzenie m_serverAddressEdit, m_serverPortEdit
        qWarning() << "WavelengthSettingsWidget::saveSettings - UI elements not initialized.";
        return;
    }

    double frequencyValueHz = 0.0;
    if (!validateFrequencyInput(frequencyValueHz)) {
        qWarning() << "WavelengthSettingsWidget::saveSettings - Invalid frequency input. Settings not saved.";
        return;
    }

    QString frequencyStringHz = QString::number(frequencyValueHz, 'f', 1);
    m_config->setPreferredStartFrequency(frequencyStringHz);

    // Usunięto zapis m_config->setRelayServerAddress(...)
    // Usunięto zapis m_config->setRelayServerPort(...)

    qDebug() << "WavelengthSettingsWidget settings prepared for saving. Preferred Frequency (Hz):" << frequencyStringHz;
}