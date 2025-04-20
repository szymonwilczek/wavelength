#include "wavelength_settings_widget.h"
#include "../../../../util/wavelength_config.h"
#include "../../../../ui/input/cyber_line_edit.h"

#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox> // Dodaj include
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDebug>

WavelengthSettingsWidget::WavelengthSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_config(WavelengthConfig::getInstance()),
      m_preferredFrequencyEdit(nullptr), // Inicjalizuj wskaźnik
      m_serverAddressEdit(nullptr),
      m_serverPortEdit(nullptr)
{
    qDebug() << "WavelengthSettingsWidget constructor start";
    setupUi();
    loadSettings(); // Załaduj ustawienia od razu po utworzeniu UI
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
    formLayout->setSpacing(15); // Zwiększony odstęp dla lepszej czytelności
    formLayout->setRowWrapPolicy(QFormLayout::WrapLongRows); // Lepsze zawijanie

    // --- Preferred Start Frequency ---
    QLabel *preferredFreqLabel = new QLabel("Preferred Start Frequency:", this);
    preferredFreqLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");

    m_preferredFrequencyEdit = new QDoubleSpinBox(this);
    m_preferredFrequencyEdit->setRange(130.0, 10000000000.0); // 130 Hz to 10 GHz
    m_preferredFrequencyEdit->setDecimals(1);
    m_preferredFrequencyEdit->setSingleStep(10.0); // Krok co 10 Hz
    m_preferredFrequencyEdit->setGroupSeparatorShown(true);
    m_preferredFrequencyEdit->setStyleSheet(
        "QDoubleSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { background-color: rgba(0, 150, 220, 100); }"
    );
    // Połącz sygnał zmiany wartości ze slotem aktualizującym sufiks
    connect(m_preferredFrequencyEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WavelengthSettingsWidget::updateFrequencySuffix);
    // Wywołaj raz, aby ustawić początkowy sufiks
    // updateFrequencySuffix(m_config->getPreferredStartFrequency()); // Użyj wartości z configu
    formLayout->addRow(preferredFreqLabel, m_preferredFrequencyEdit);


    // --- Server Address ---
    QLabel *serverAddrLabel = new QLabel("Server Address:", this);
    serverAddrLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    m_serverAddressEdit = new CyberLineEdit(this);
    m_serverAddressEdit->setPlaceholderText("Enter server address");
    QLabel *serverAddrInfo = new QLabel("Leave unchanged unless using a custom server.", this);
    serverAddrInfo->setStyleSheet("color: #aaaaaa; background-color: transparent; font-family: Consolas; font-size: 8pt; margin-left: 5px;");
    serverAddrInfo->setWordWrap(true);

    QVBoxLayout* addrLayout = new QVBoxLayout;
    addrLayout->setSpacing(3);
    addrLayout->setContentsMargins(0,0,0,0);
    addrLayout->addWidget(m_serverAddressEdit);
    addrLayout->addWidget(serverAddrInfo);
    formLayout->addRow(serverAddrLabel, addrLayout);


    // --- Server Port ---
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
    portLayout->setSpacing(3);
    portLayout->setContentsMargins(0,0,0,0);
    portLayout->addWidget(m_serverPortEdit);
    portLayout->addWidget(serverPortInfo);
    formLayout->addRow(serverPortLabel, portLayout);


    layout->addLayout(formLayout);
    layout->addStretch();
    qDebug() << "WavelengthSettingsWidget setupUi end";
}

// Implementacja slotu do aktualizacji sufiksu
void WavelengthSettingsWidget::updateFrequencySuffix(double value) {
    if (!m_preferredFrequencyEdit) return;

    QString suffix = " Hz";
    if (value >= 1000000000.0) { // GHz
        suffix = " GHz";
        // Opcjonalnie: można by skalować wyświetlaną wartość, ale QDoubleSpinBox tego nie wspiera bezpośrednio
        // m_preferredFrequencyEdit->setDecimals(3); // Większa precyzja dla GHz?
    } else if (value >= 1000000.0) { // MHz
        suffix = " MHz";
        // m_preferredFrequencyEdit->setDecimals(3);
    } else if (value >= 1000.0) { // kHz
        suffix = " kHz";
        // m_preferredFrequencyEdit->setDecimals(1);
    } else { // Hz
         m_preferredFrequencyEdit->setDecimals(1);
    }
    m_preferredFrequencyEdit->setSuffix(suffix);
}


void WavelengthSettingsWidget::loadSettings() {
    if (!m_preferredFrequencyEdit || !m_serverAddressEdit || !m_serverPortEdit) {
        qWarning() << "WavelengthSettingsWidget::loadSettings - UI elements not initialized.";
        return;
    }
    m_preferredFrequencyEdit->setValue(m_config->getPreferredStartFrequency().toDouble());
    m_serverAddressEdit->setText(m_config->getRelayServerAddress());
    m_serverPortEdit->setValue(m_config->getRelayServerPort());
    // Upewnij się, że sufiks jest poprawny po załadowaniu
    updateFrequencySuffix(m_preferredFrequencyEdit->value());
    qDebug() << "WavelengthSettingsWidget settings loaded.";
}

void WavelengthSettingsWidget::saveSettings() {
    if (!m_preferredFrequencyEdit || !m_serverAddressEdit || !m_serverPortEdit) {
        qWarning() << "WavelengthSettingsWidget::saveSettings - UI elements not initialized.";
        return;
    }
    m_config->setPreferredStartFrequency(m_preferredFrequencyEdit->text());
    m_config->setRelayServerAddress(m_serverAddressEdit->text());
    m_config->setRelayServerPort(m_serverPortEdit->value());
    qDebug() << "WavelengthSettingsWidget settings prepared for saving.";
    // Nie wywołujemy m_config->saveSettings() tutaj, zrobi to główny widok
}