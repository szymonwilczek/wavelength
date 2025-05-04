#ifndef WAVELENGTH_SETTINGS_WIDGET_H
#define WAVELENGTH_SETTINGS_WIDGET_H

#include <QWidget>

// Forward declarations
class WavelengthConfig;
class QLineEdit;
class QComboBox;
class QLabel;
class QVBoxLayout;
class QFormLayout;
class QHBoxLayout;

/**
 * @brief A widget for configuring Wavelength-specific settings, primarily the preferred start frequency.
 *
 * This widget provides UI elements (line edit, combo box) to allow the user to set their
 * preferred starting frequency when searching for available wavelengths. It includes input
 * validation to ensure the frequency is within acceptable limits (130 Hz to 999.9 MHz).
 * It interacts with the WavelengthConfig singleton to load and save this setting.
 */
class WavelengthSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs the WavelengthSettingsWidget.
     * Initializes the UI, loads the current preferred frequency from WavelengthConfig.
     * @param parent Optional parent widget.
     */
    explicit WavelengthSettingsWidget(QWidget *parent = nullptr);

    /**
     * @brief Default destructor.
     */
    ~WavelengthSettingsWidget() override = default;

    /**
     * @brief Loads the current preferred start frequency from WavelengthConfig and updates the UI elements.
     * Converts the stored frequency (in Hz) to an appropriate unit (Hz, kHz, MHz) for display.
     */
    void LoadSettings() const;

    /**
     * @brief Validates the frequency input and saves the preferred start frequency back to WavelengthConfig.
     * Converts the user input (value and unit) back to Hz before saving. Displays warnings if input is invalid.
     */
    void SaveSettings();

private:
    /**
     * @brief Creates and arranges all the UI elements (labels, inputs, layouts) for the widget.
     * Sets up the frequency value input field with a validator and the unit selection combo box.
     */
    void SetupUi();

    /**
     * @brief Validates the frequency value and unit entered by the user.
     * Checks if the value is a positive number, within the unit's limit (<= 999.9), and if the
     * resulting frequency in Hz is within the global limits (>= 130 Hz, <= 999.9 MHz).
     * Displays warning messages for invalid input.
     * @param[out] hz The validated frequency converted to Hertz if validation succeeds.
     * @return True if the input is valid, false otherwise.
     */
    bool ValidateFrequencyInput(double& hz);

    /** @brief Pointer to the WavelengthConfig singleton instance. */
    WavelengthConfig *config_;

    /** @brief Input field for the numerical value of the preferred frequency. */
    QLineEdit *frequency_value_edit_;
    /** @brief Combo box for selecting the unit (Hz, kHz, MHz) of the preferred frequency. */
    QComboBox *frequency_unit_combo_;
};

#endif // WAVELENGTH_SETTINGS_WIDGET_H