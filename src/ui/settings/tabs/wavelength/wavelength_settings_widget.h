// filepath: c:\Users\szymo\Documents\GitHub\wavelength\wavelength\view\settings\tabs\wavelength_tab\wavelength_settings_widget.h
#ifndef WAVELENGTH_SETTINGS_WIDGET_H
#define WAVELENGTH_SETTINGS_WIDGET_H

// Usunięto: #include <QDoubleSpinBox>
#include <QWidget>

class WavelengthConfig;
// Usunięto: class CyberLineEdit;
class QLineEdit;
class QComboBox;
// Usunięto: class QSpinBox;
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