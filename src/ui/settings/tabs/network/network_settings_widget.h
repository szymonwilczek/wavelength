#ifndef ADVANCED_SETTINGS_WIDGET_H
#define ADVANCED_SETTINGS_WIDGET_H

#include <QWidget>

class WavelengthConfig;
class CyberLineEdit;
class QSpinBox;
class QVBoxLayout;
class QFormLayout;

/**
 * @brief A widget for configuring advanced network settings for the application.
 *
 * This widget provides UI elements (line edits, spin boxes) to allow the user
 * to customize network parameters such as the relay server address and port,
 * connection timeout, keep-alive interval, and maximum reconnection attempts.
 * It interacts with the WavelengthConfig singleton to load and save these settings.
 * Changes made via the UI are saved back to WavelengthConfig when SaveSettings() is called.
 */
class NetworkSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs the NetworkSettingsWidget.
     * Initializes the UI, loads current settings from WavelengthConfig.
     * @param parent Optional parent widget.
     */
    explicit NetworkSettingsWidget(QWidget *parent = nullptr);

    /**
     * @brief Default destructor.
     */
    ~NetworkSettingsWidget() override = default;

    /**
     * @brief Loads the current network settings from WavelengthConfig and updates the UI elements.
     */
    void LoadSettings() const;

    /**
     * @brief Saves the currently configured settings from the UI back to WavelengthConfig.
     */
    void SaveSettings() const;

private:
    /**
     * @brief Creates and arranges all the UI elements (labels, inputs, layouts) for the widget.
     */
    void SetupUi();

    /** @brief Pointer to the WavelengthConfig singleton instance. */
    WavelengthConfig *config_;
    /** @brief Input field for the relay server address. */
    CyberLineEdit *server_address_edit_;
    /** @brief Spin box for the relay server port. */
    QSpinBox *server_port_edit_;
    /** @brief Spin box for the connection timeout in milliseconds. */
    QSpinBox *connection_timeout_edit_;
    /** @brief Spin box for the keep-alive interval in milliseconds. */
    QSpinBox *keep_alive_interval_edit_;
    /** @brief Spin box for the maximum number of reconnection attempts. */
    QSpinBox *max_reconnect_attempts_edit_;
};

#endif // ADVANCED_SETTINGS_WIDGET_H
