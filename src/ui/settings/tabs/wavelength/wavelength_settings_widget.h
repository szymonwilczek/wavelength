#ifndef WAVELENGTH_SETTINGS_WIDGET_H
#define WAVELENGTH_SETTINGS_WIDGET_H

#include <QWidget>

class WavelengthConfig;
class QLineEdit;
class QComboBox;
class QLabel;
class QVBoxLayout;
class QFormLayout;
class QHBoxLayout;

class WavelengthSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit WavelengthSettingsWidget(QWidget *parent = nullptr);
    ~WavelengthSettingsWidget() override = default;

    void loadSettings();
    void saveSettings();

private:
    void setupUi();
    bool validateFrequencyInput(double& valueHz);

    WavelengthConfig *m_config;

    // Kontrolki UI dla tej zakładki
    QLineEdit *m_frequencyValueEdit;
    QComboBox *m_frequencyUnitCombo;
    // Usunięto: CyberLineEdit *m_serverAddressEdit;
    // Usunięto: QSpinBox *m_serverPortEdit;
};

#endif // WAVELENGTH_SETTINGS_WIDGET_H