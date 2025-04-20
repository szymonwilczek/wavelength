#include "wavelength_settings_widget.h"
#include "../../../../util/wavelength_config.h"
#include "../../../../ui/input/cyber_line_edit.h"

#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDebug>

WavelengthSettingsWidget::WavelengthSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_config(WavelengthConfig::getInstance()),
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
    // Używamy this zamiast 'tab'
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    QLabel *infoLabel = new QLabel("Configure server connection settings", this);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(infoLabel);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(15);

    // Inicjalizujemy kontrolki jako składowe klasy
    m_serverAddressEdit = new CyberLineEdit(this);
    m_serverAddressEdit->setPlaceholderText("Enter server address");
    formLayout->addRow("Server Address:", m_serverAddressEdit);

    m_serverPortEdit = new QSpinBox(this);
    m_serverPortEdit->setRange(1, 65535);
    m_serverPortEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
        "QSpinBox::up-button, QSpinBox::down-button { background-color: rgba(0, 150, 220, 100); }"
    );
    formLayout->addRow("Server Port:", m_serverPortEdit);

    layout->addLayout(formLayout);
    layout->addStretch();
    qDebug() << "WavelengthSettingsWidget setupUi end";
}

void WavelengthSettingsWidget::loadSettings() {
    if (!m_serverAddressEdit || !m_serverPortEdit) {
        qWarning() << "WavelengthSettingsWidget::loadSettings - UI elements not initialized.";
        return;
    }
    m_serverAddressEdit->setText(m_config->getRelayServerAddress());
    m_serverPortEdit->setValue(m_config->getRelayServerPort());
    qDebug() << "WavelengthSettingsWidget settings loaded.";
}

void WavelengthSettingsWidget::saveSettings() {
    if (!m_serverAddressEdit || !m_serverPortEdit) {
        qWarning() << "WavelengthSettingsWidget::saveSettings - UI elements not initialized.";
        return;
    }
    m_config->setRelayServerAddress(m_serverAddressEdit->text());
    m_config->setRelayServerPort(m_serverPortEdit->value());
    qDebug() << "WavelengthSettingsWidget settings prepared for saving.";
    // Nie wywołujemy m_config->saveSettings() tutaj, zrobi to główny widok
}