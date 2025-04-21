#ifndef WAVELENGTH_SETTINGS_WIDGET_H
#define WAVELENGTH_SETTINGS_WIDGET_H

// Usunięto: #include <QDoubleSpinBox>
#include <QWidget>

class WavelengthConfig;
class CyberLineEdit; // Zmienimy na standardowy QLineEdit dla uproszczenia walidacji
class QLineEdit;     // Dodano
class QComboBox;     // Dodano
class QSpinBox;
class QLabel;
class QVBoxLayout;
class QFormLayout;
class QHBoxLayout;   // Dodano

class WavelengthSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit WavelengthSettingsWidget(QWidget *parent = nullptr);
    ~WavelengthSettingsWidget() override = default;

    void loadSettings();
    void saveSettings();

private:
    void setupUi();
    // Usunięto: void updateFrequencySuffix(double value);
    bool validateFrequencyInput(double& valueHz); // Nowa funkcja pomocnicza do walidacji

    WavelengthConfig *m_config;

    // Kontrolki UI dla tej zakładki
    // Usunięto: QDoubleSpinBox *m_preferredFrequencyEdit;
    QLineEdit *m_frequencyValueEdit;     // Zamiast QDoubleSpinBox
    QComboBox *m_frequencyUnitCombo;     // Do wyboru jednostki
    CyberLineEdit *m_serverAddressEdit;  // Pozostaje bez zmian
    QSpinBox *m_serverPortEdit;          // Pozostaje bez zmian
};

#endif // WAVELENGTH_SETTINGS_WIDGET_H