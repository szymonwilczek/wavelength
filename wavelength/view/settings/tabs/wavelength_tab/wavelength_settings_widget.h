#ifndef WAVELENGTH_SETTINGS_WIDGET_H
#define WAVELENGTH_SETTINGS_WIDGET_H

#include <QDoubleSpinBox>
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
    void updateFrequencySuffix(double value);

    WavelengthConfig *m_config;

    // Kontrolki UI dla tej zakładki
    QDoubleSpinBox *m_preferredFrequencyEdit;
    CyberLineEdit *m_serverAddressEdit;
    QSpinBox *m_serverPortEdit;
};

#endif // WAVELENGTH_SETTINGS_WIDGET_H