#include "network_settings_widget.h"
#include "../../../../util/wavelength_config.h"
#include "../../../../ui/input/cyber_line_edit.h" // Potrzebne dla CyberLineEdit

#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDebug>

NetworkSettingsWidget::NetworkSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_config(WavelengthConfig::getInstance()),
      m_serverAddressEdit(nullptr),
      m_serverPortEdit(nullptr),
      m_connectionTimeoutEdit(nullptr),
      m_keepAliveIntervalEdit(nullptr),
      m_maxReconnectAttemptsEdit(nullptr)
{
    qDebug() << "AdvancedSettingsWidget constructor start";
    setupUi();
    loadSettings();
    qDebug() << "AdvancedSettingsWidget constructor end";
}

void NetworkSettingsWidget::setupUi() {
    qDebug() << "AdvancedSettingsWidget setupUi start";
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    QLabel *infoLabel = new QLabel("Configure network connection parameters", this);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(infoLabel);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(15);
    formLayout->setRowWrapPolicy(QFormLayout::WrapLongRows); // Upewnij się, że jest to ustawione

    // --- Server Address --- (Przeniesione z WavelengthSettingsWidget)
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

    // --- Server Port --- (Przeniesione z WavelengthSettingsWidget)
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

    // --- Connection Timeout --- (Przeniesione z SettingsView::setupNetworkTab)
    QLabel *connTimeoutLabel = new QLabel("Connection Timeout:", this);
    connTimeoutLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;"); // Styl z innych etykiet
    m_connectionTimeoutEdit = new QSpinBox(this);
    m_connectionTimeoutEdit->setRange(1000, 60000);
    m_connectionTimeoutEdit->setSingleStep(500);
    m_connectionTimeoutEdit->setSuffix(" ms");
    m_connectionTimeoutEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }" // Styl z innych SpinBoxów
    );
    formLayout->addRow(connTimeoutLabel, m_connectionTimeoutEdit);

    // --- Keep Alive Interval --- (Przeniesione z SettingsView::setupNetworkTab)
    QLabel *keepAliveLabel = new QLabel("Keep Alive Interval:", this);
    keepAliveLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    m_keepAliveIntervalEdit = new QSpinBox(this);
    m_keepAliveIntervalEdit->setRange(1000, 60000);
    m_keepAliveIntervalEdit->setSingleStep(500);
    m_keepAliveIntervalEdit->setSuffix(" ms");
    m_keepAliveIntervalEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
    );
    formLayout->addRow(keepAliveLabel, m_keepAliveIntervalEdit);

    // --- Max Reconnect Attempts --- (Przeniesione z SettingsView::setupNetworkTab)
    QLabel *reconnectLabel = new QLabel("Max Reconnect Attempts:", this);
    reconnectLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    m_maxReconnectAttemptsEdit = new QSpinBox(this);
    m_maxReconnectAttemptsEdit->setRange(0, 20);
    m_maxReconnectAttemptsEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
    );
    formLayout->addRow(reconnectLabel, m_maxReconnectAttemptsEdit);

    layout->addLayout(formLayout);
    layout->addStretch();
    qDebug() << "AdvancedSettingsWidget setupUi end";
}

void NetworkSettingsWidget::loadSettings() {
    if (!m_serverAddressEdit || !m_serverPortEdit || !m_connectionTimeoutEdit || !m_keepAliveIntervalEdit || !m_maxReconnectAttemptsEdit) {
        qWarning() << "AdvancedSettingsWidget::loadSettings - UI elements not initialized.";
        return;
    }
    m_serverAddressEdit->setText(m_config->getRelayServerAddress());
    m_serverPortEdit->setValue(m_config->getRelayServerPort());
    m_connectionTimeoutEdit->setValue(m_config->getConnectionTimeout());
    m_keepAliveIntervalEdit->setValue(m_config->getKeepAliveInterval());
    m_maxReconnectAttemptsEdit->setValue(m_config->getMaxReconnectAttempts());
    qDebug() << "AdvancedSettingsWidget settings loaded.";
}

void NetworkSettingsWidget::saveSettings() {
    if (!m_serverAddressEdit || !m_serverPortEdit || !m_connectionTimeoutEdit || !m_keepAliveIntervalEdit || !m_maxReconnectAttemptsEdit) {
        qWarning() << "AdvancedSettingsWidget::saveSettings - UI elements not initialized.";
        return;
    }
    m_config->setRelayServerAddress(m_serverAddressEdit->text());
    m_config->setRelayServerPort(m_serverPortEdit->value());
    m_config->setConnectionTimeout(m_connectionTimeoutEdit->value());
    m_config->setKeepAliveInterval(m_keepAliveIntervalEdit->value());
    m_config->setMaxReconnectAttempts(m_maxReconnectAttemptsEdit->value());
    qDebug() << "AdvancedSettingsWidget settings prepared for saving.";
    // Nie wywołujemy m_config->saveSettings() tutaj, zrobi to główny widok
}