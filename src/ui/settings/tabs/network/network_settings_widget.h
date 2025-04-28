#ifndef ADVANCED_SETTINGS_WIDGET_H
#define ADVANCED_SETTINGS_WIDGET_H

#include <QWidget>

class WavelengthConfig;
class CyberLineEdit;
class QSpinBox;
class QVBoxLayout;
class QFormLayout;

class NetworkSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit NetworkSettingsWidget(QWidget *parent = nullptr);
    ~NetworkSettingsWidget() override = default;

    void loadSettings();
    void saveSettings();

private:
    void setupUi();

    WavelengthConfig *m_config;

    // Kontrolki UI dla tej zakładki
    CyberLineEdit *m_serverAddressEdit;
    QSpinBox *m_serverPortEdit;
    QSpinBox *m_connectionTimeoutEdit;
    QSpinBox *m_keepAliveIntervalEdit;
    QSpinBox *m_maxReconnectAttemptsEdit;
};

#endif // ADVANCED_SETTINGS_WIDGET_H