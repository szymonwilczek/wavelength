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

class WavelengthSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit WavelengthSettingsWidget(QWidget *parent = nullptr);
    ~WavelengthSettingsWidget() override = default;

    void LoadSettings() const;
    void SaveSettings();

private:
    void setupUi();
    bool ValidateFrequencyInput(double& hz);

    WavelengthConfig *config_;

    QLineEdit *frequency_value_edit_;
    QComboBox *frequency_unit_combo_;
};

#endif // WAVELENGTH_SETTINGS_WIDGET_H