#ifndef WAVELENGTH_SETTINGS_WIDGET_H
#define WAVELENGTH_SETTINGS_WIDGET_H

#include <QWidget>

class WavelengthConfig;
class CyberLineEdit;
class QSpinBox;
class QLabel;
class QVBoxLayout;
class QFormLayout;

class WavelengthSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit WavelengthSettingsWidget(QWidget *parent = nullptr);
    ~WavelengthSettingsWidget() override = default;

    void loadSettings();
    void saveSettings();

private:
    void setupUi();

    WavelengthConfig *m_config;

    // Kontrolki UI dla tej zakładki
    CyberLineEdit *m_serverAddressEdit;
    QSpinBox *m_serverPortEdit;
};

#endif // WAVELENGTH_SETTINGS_WIDGET_H