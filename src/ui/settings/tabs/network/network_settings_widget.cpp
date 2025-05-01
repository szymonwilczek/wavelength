#include "network_settings_widget.h"
#include "../../../../app/wavelength_config.h"
#include "../../../../ui/input/cyber_line_edit.h"

#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDebug>

NetworkSettingsWidget::NetworkSettingsWidget(QWidget *parent)
    : QWidget(parent),
      config_(WavelengthConfig::GetInstance()),
      server_address_edit_(nullptr),
      server_port_edit_(nullptr),
      connection_timeout_edit_(nullptr),
      keep_alive_interval_edit_(nullptr),
      max_reconnect_attempts_edit_(nullptr)
{
    qDebug() << "AdvancedSettingsWidget constructor start";
    SetupUi();
    LoadSettings();
    qDebug() << "AdvancedSettingsWidget constructor end";
}

void NetworkSettingsWidget::SetupUi() {
    qDebug() << "AdvancedSettingsWidget setupUi start";
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    const auto info_label = new QLabel("Configure network connection parameters", this);
    info_label->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(info_label);

    const auto form_layout = new QFormLayout();
    form_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form_layout->setSpacing(15);
    form_layout->setRowWrapPolicy(QFormLayout::WrapLongRows); // Upewnij się, że jest to ustawione

    // --- Server Address --- (Przeniesione z WavelengthSettingsWidget)
    const auto server_addr_label = new QLabel("Server Address:", this);
    server_addr_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    server_address_edit_ = new CyberLineEdit(this);
    server_address_edit_->setPlaceholderText("Enter server address");
    const auto server_addr_info = new QLabel("Leave unchanged unless using a custom server.", this);
    server_addr_info->setStyleSheet("color: #aaaaaa; background-color: transparent; font-family: Consolas; font-size: 8pt; margin-left: 5px;");
    server_addr_info->setWordWrap(true);
    const auto addr_layout = new QVBoxLayout;
    addr_layout->setSpacing(3); addr_layout->setContentsMargins(0,0,0,0);
    addr_layout->addWidget(server_address_edit_); addr_layout->addWidget(server_addr_info);
    form_layout->addRow(server_addr_label, addr_layout);

    // --- Server Port --- (Przeniesione z WavelengthSettingsWidget)
    const auto server_port_label = new QLabel("Server Port:", this);
    server_port_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    server_port_edit_ = new QSpinBox(this);
    server_port_edit_->setRange(1, 65535);
    server_port_edit_->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
        "QSpinBox::up-button, QSpinBox::down-button { background-color: rgba(0, 150, 220, 100); }"
    );
    const auto server_port_info = new QLabel("Mainly for debugging. Default server ignores this if address is unchanged.", this);
    server_port_info->setStyleSheet("color: #aaaaaa; background-color: transparent; font-family: Consolas; font-size: 8pt; margin-left: 5px;");
    server_port_info->setWordWrap(true);
    const auto port_layout = new QVBoxLayout;
    port_layout->setSpacing(3); port_layout->setContentsMargins(0,0,0,0);
    port_layout->addWidget(server_port_edit_); port_layout->addWidget(server_port_info);
    form_layout->addRow(server_port_label, port_layout);

    // --- Connection Timeout --- (Przeniesione z SettingsView::setupNetworkTab)
    const auto conn_timeout_label = new QLabel("Connection Timeout:", this);
    conn_timeout_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;"); // Styl z innych etykiet
    connection_timeout_edit_ = new QSpinBox(this);
    connection_timeout_edit_->setRange(1000, 60000);
    connection_timeout_edit_->setSingleStep(500);
    connection_timeout_edit_->setSuffix(" ms");
    connection_timeout_edit_->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }" // Styl z innych SpinBoxów
    );
    form_layout->addRow(conn_timeout_label, connection_timeout_edit_);

    // --- Keep Alive Interval --- (Przeniesione z SettingsView::setupNetworkTab)
    const auto keep_alive_label = new QLabel("Keep Alive Interval:", this);
    keep_alive_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    keep_alive_interval_edit_ = new QSpinBox(this);
    keep_alive_interval_edit_->setRange(1000, 60000);
    keep_alive_interval_edit_->setSingleStep(500);
    keep_alive_interval_edit_->setSuffix(" ms");
    keep_alive_interval_edit_->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
    );
    form_layout->addRow(keep_alive_label, keep_alive_interval_edit_);

    // --- Max Reconnect Attempts --- (Przeniesione z SettingsView::setupNetworkTab)
    const auto reconnect_label = new QLabel("Max Reconnect Attempts:", this);
    reconnect_label->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    max_reconnect_attempts_edit_ = new QSpinBox(this);
    max_reconnect_attempts_edit_->setRange(0, 20);
    max_reconnect_attempts_edit_->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; font-family: Consolas; }"
    );
    form_layout->addRow(reconnect_label, max_reconnect_attempts_edit_);

    layout->addLayout(form_layout);
    layout->addStretch();
    qDebug() << "AdvancedSettingsWidget setupUi end";
}

void NetworkSettingsWidget::LoadSettings() const {
    if (!server_address_edit_ || !server_port_edit_ || !connection_timeout_edit_ || !keep_alive_interval_edit_ || !max_reconnect_attempts_edit_) {
        qWarning() << "AdvancedSettingsWidget::loadSettings - UI elements not initialized.";
        return;
    }
    server_address_edit_->setText(config_->GetRelayServerAddress());
    server_port_edit_->setValue(config_->GetRelayServerPort());
    connection_timeout_edit_->setValue(config_->GetConnectionTimeout());
    keep_alive_interval_edit_->setValue(config_->GetKeepAliveInterval());
    max_reconnect_attempts_edit_->setValue(config_->GetMaxReconnectAttempts());
    qDebug() << "AdvancedSettingsWidget settings loaded.";
}

void NetworkSettingsWidget::SaveSettings() const {
    if (!server_address_edit_ || !server_port_edit_ || !connection_timeout_edit_ || !keep_alive_interval_edit_ || !max_reconnect_attempts_edit_) {
        qWarning() << "AdvancedSettingsWidget::saveSettings - UI elements not initialized.";
        return;
    }
    config_->SetRelayServerAddress(server_address_edit_->text());
    config_->SetRelayServerPort(server_port_edit_->value());
    config_->SetConnectionTimeout(connection_timeout_edit_->value());
    config_->SetKeepAliveInterval(keep_alive_interval_edit_->value());
    config_->SetMaxReconnectAttempts(max_reconnect_attempts_edit_->value());
    qDebug() << "AdvancedSettingsWidget settings prepared for saving.";
}
