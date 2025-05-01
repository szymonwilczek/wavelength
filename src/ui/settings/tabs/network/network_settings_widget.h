#ifndef ADVANCED_SETTINGS_WIDGET_H
#define ADVANCED_SETTINGS_WIDGET_H

#include <QWidget>

class WavelengthConfig;
class CyberLineEdit;
class QSpinBox;
class QVBoxLayout;
class QFormLayout;

class NetworkSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit NetworkSettingsWidget(QWidget *parent = nullptr);
    ~NetworkSettingsWidget() override = default;

    void LoadSettings() const;
    void SaveSettings() const;

private:
    void SetupUi();

    WavelengthConfig *config_;

    CyberLineEdit *server_address_edit_;
    QSpinBox *server_port_edit_;
    QSpinBox *connection_timeout_edit_;
    QSpinBox *keep_alive_interval_edit_;
    QSpinBox *max_reconnect_attempts_edit_;
};

#endif // ADVANCED_SETTINGS_WIDGET_H