#include "wavelength_settings_widget.h"
#include "../../../../util/wavelength_config.h"
#include "../../../../ui/input/cyber_line_edit.h" // Nadal używane dla adresu serwera

#include <QLabel>
#include <QSpinBox>
// Usunięto: #include <QDoubleSpinBox>
#include <QLineEdit>     // Dodano
#include <QComboBox>     // Dodano
#include <QHBoxLayout>   // Dodano
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDebug>
#include <QString>
#include <QDoubleValidator> // Dodano do walidacji QLineEdit
#include <QMessageBox>      // Dodano do wyświetlania błędów walidacji
#include <QLocale>

// Definicje stałych dla walidacji
namespace FreqLimits {
    const double MIN_HZ = 130.0;
    // Maksymalna wartość, jaką można wpisać dla danej jednostki (np. 999.9 kHz)
    const double MAX_VALUE_PER_UNIT = 999.9;
    // Maksymalna częstotliwość w Hz (odpowiada 999.9 MHz)
    const double MAX_HZ = 999900000.0;
}

WavelengthSettingsWidget::WavelengthSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_config(WavelengthConfig::getInstance()),
      m_frequencyValueEdit(nullptr), // Inicjalizacja nowych wskaźników
      m_frequencyUnitCombo(nullptr),
      m_serverAddressEdit(nullptr),
      m_serverPortEdit(nullptr)
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

    QLabel *infoLabel = new QLabel("Configure server connection and frequency preferences", this);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(infoLabel);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(15);
    formLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);

    // --- Preferred Start Frequency ---
    QLabel *preferredFreqLabel = new QLabel("Preferred Start Frequency:", this);
    preferredFreqLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    // Kontener poziomy dla pola wpisu i listy jednostek
    QHBoxLayout *freqLayout = new QHBoxLayout();
    freqLayout->setSpacing(5);

    m_frequencyValueEdit = new QLineEdit(this);
    m_frequencyValueEdit->setPlaceholderText("e.g., 150.5 or 98.7");
    // Podstawowa walidacja - akceptuj liczby zmiennoprzecinkowe
    // Użyj QLocale::C aby kropka była separatorem dziesiętnym
    QDoubleValidator *validator = new QDoubleValidator(0.0, FreqLimits::MAX_VALUE_PER_UNIT * 10, 3, this); // Pozwól na więcej miejsc tymczasowo
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::C); // Używaj kropki jako separatora
    m_frequencyValueEdit->setValidator(validator);
    m_frequencyValueEdit->setStyleSheet(
        "QLineEdit { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
    );
    freqLayout->addWidget(m_frequencyValueEdit, 3); // Pole wartości zajmuje więcej miejsca

    m_frequencyUnitCombo = new QComboBox(this);
    m_frequencyUnitCombo->addItems({"Hz", "kHz", "MHz"});
    m_frequencyUnitCombo->setStyleSheet(
        "QComboBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
        "QComboBox::drop-down { border: none; background-color: rgba(0, 150, 220, 100); }"
        "QComboBox QAbstractItemView { background-color: #0A1928; color: #00eeff; selection-background-color: #4682B4; }" // Styl listy rozwijanej
    );
    freqLayout->addWidget(m_frequencyUnitCombo, 1); // Lista jednostek zajmuje mniej miejsca

    formLayout->addRow(preferredFreqLabel, freqLayout); // Dodaj układ poziomy do formularza

    // --- Server Address --- (bez zmian)
    QLabel *serverAddrLabel = new QLabel("Server Address:", this);
    serverAddrLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    m_serverAddressEdit = new CyberLineEdit(this);
    m_serverAddressEdit->setPlaceholderText("Enter server address");
    QLabel *serverAddrInfo = new QLabel("Leave unchanged unless using a custom server.", this);
    serverAddrInfo->setStyleSheet("color: #aaaaaa; background-color: transparent; font-family: Consolas; font-size: 8pt; margin-left: 5px;");
    serverAddrInfo->setWordWrap(true);
    QVBoxLayout* addrLayout = new QVBoxLayout;
    addrLayout->setSpacing(3); addrLayout->setContentsMargins(0,0,0,0);
    addrLayout->addWidget(m_serverAddressEdit); addrLayout->addWidget(serverAddrInfo);
    formLayout->addRow(serverAddrLabel, addrLayout);

    // --- Server Port --- (bez zmian)
    QLabel *serverPortLabel = new QLabel("Server Port:", this);
    serverPortLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    m_serverPortEdit = new QSpinBox(this);
    m_serverPortEdit->setRange(1, 65535);
    m_serverPortEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
        "QSpinBox::up-button, QSpinBox::down-button { background-color: rgba(0, 150, 220, 100); }"
    );
    QLabel *serverPortInfo = new QLabel("Mainly for debugging. Default server ignores this if address is unchanged.", this);
    serverPortInfo->setStyleSheet("color: #aaaaaa; background-color: transparent; font-family: Consolas; font-size: 8pt; margin-left: 5px;");
    serverPortInfo->setWordWrap(true);
    QVBoxLayout* portLayout = new QVBoxLayout;
    portLayout->setSpacing(3); portLayout->setContentsMargins(0,0,0,0);
    portLayout->addWidget(m_serverPortEdit); portLayout->addWidget(serverPortInfo);
    formLayout->addRow(serverPortLabel, portLayout);

    layout->addLayout(formLayout);
    layout->addStretch();
    qDebug() << "WavelengthSettingsWidget setupUi end";
}

// Usunięto funkcję updateFrequencySuffix

void WavelengthSettingsWidget::loadSettings() {
    if (!m_frequencyValueEdit || !m_frequencyUnitCombo || !m_serverAddressEdit || !m_serverPortEdit) {
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

    // Wybierz najlepszą jednostkę do wyświetlenia
    double displayValue = freqValueHz;
    int unitIndex = 0; // Domyślnie Hz

    if (freqValueHz >= 1000000.0 && freqValueHz <= FreqLimits::MAX_HZ) { // MHz
        displayValue = freqValueHz / 1000000.0;
        unitIndex = 2; // MHz
    } else if (freqValueHz >= 1000.0) { // kHz
        displayValue = freqValueHz / 1000.0;
        unitIndex = 1; // kHz
    }

    // Ustaw wartości w kontrolkach
    // Użyj QString::number z 'f' i odpowiednią precyzją, lokalizacja C dla kropki
    m_frequencyValueEdit->setText(QString::number(displayValue, 'f', (unitIndex > 0) ? 3 : 1)); // Więcej precyzji dla kHz/MHz
    m_frequencyUnitCombo->setCurrentIndex(unitIndex);

    m_serverAddressEdit->setText(m_config->getRelayServerAddress());
    m_serverPortEdit->setValue(m_config->getRelayServerPort());

    qDebug() << "WavelengthSettingsWidget settings loaded. Displayed:" << displayValue << m_frequencyUnitCombo->currentText();
}

// Nowa funkcja pomocnicza do walidacji
bool WavelengthSettingsWidget::validateFrequencyInput(double& valueHz) {
    if (!m_frequencyValueEdit || !m_frequencyUnitCombo) return false;

    QString valueStr = m_frequencyValueEdit->text();
    QString unitStr = m_frequencyUnitCombo->currentText();
    bool ok;

    QLocale cLocale(QLocale::C); // Utwórz obiekt C locale
    double valueDouble = cLocale.toDouble(valueStr, &ok); // Wywołaj toDouble na obiekcie

    if (!ok || valueDouble <= 0) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid positive number for the frequency value.");
        m_frequencyValueEdit->setFocus();
        m_frequencyValueEdit->selectAll();
        return false;
    }

    // Sprawdź, czy wpisana wartość nie przekracza limitu dla jednostki (np. 999.9 kHz)
    if (valueDouble > FreqLimits::MAX_VALUE_PER_UNIT) {
         QMessageBox::warning(this, "Invalid Input", QString("The maximum value for %1 is %2.").arg(unitStr).arg(FreqLimits::MAX_VALUE_PER_UNIT));
         m_frequencyValueEdit->setFocus();
         m_frequencyValueEdit->selectAll();
         return false;
    }


    // Oblicz wartość w Hz
    if (unitStr == "kHz") {
        valueHz = valueDouble * 1000.0;
    } else if (unitStr == "MHz") {
        valueHz = valueDouble * 1000000.0;
    } else { // Hz
        valueHz = valueDouble;
    }

    // Sprawdź dolny limit (130 Hz)
    if (valueHz < FreqLimits::MIN_HZ) {
        QMessageBox::warning(this, "Invalid Input", QString("The minimum frequency allowed is %1 Hz.").arg(FreqLimits::MIN_HZ));
         m_frequencyValueEdit->setFocus();
         m_frequencyValueEdit->selectAll();
        return false;
    }

    // Sprawdź górny limit (999.9 MHz) - technicznie pokryty przez MAX_VALUE_PER_UNIT, ale dla pewności
     if (valueHz > FreqLimits::MAX_HZ) {
         QMessageBox::warning(this, "Invalid Input", QString("The maximum frequency allowed is %1 MHz.").arg(FreqLimits::MAX_VALUE_PER_UNIT));
         m_frequencyValueEdit->setFocus();
         m_frequencyValueEdit->selectAll();
         return false;
     }


    // Walidacja zakończona sukcesem
    return true;
}


void WavelengthSettingsWidget::saveSettings() {
    if (!m_frequencyValueEdit || !m_frequencyUnitCombo || !m_serverAddressEdit || !m_serverPortEdit) {
        qWarning() << "WavelengthSettingsWidget::saveSettings - UI elements not initialized.";
        return;
    }

    double frequencyValueHz = 0.0;
    // Wywołaj walidację. Jeśli się nie powiedzie, zakończ.
    if (!validateFrequencyInput(frequencyValueHz)) {
        qWarning() << "WavelengthSettingsWidget::saveSettings - Invalid frequency input. Settings not saved.";
        return; // Nie zapisuj, jeśli walidacja się nie powiodła
    }

    // Konwertuj poprawną wartość Hz na QString z jednym miejscem po przecinku
    QString frequencyStringHz = QString::number(frequencyValueHz, 'f', 1);

    // Zapisz string reprezentujący Hz do konfiguracji
    m_config->setPreferredStartFrequency(frequencyStringHz);

    m_config->setRelayServerAddress(m_serverAddressEdit->text());
    m_config->setRelayServerPort(m_serverPortEdit->value());

    qDebug() << "WavelengthSettingsWidget settings prepared for saving. Preferred Frequency (Hz):" << frequencyStringHz;
    // Nie wywołujemy m_config->saveSettings() tutaj, zrobi to główny widok
}