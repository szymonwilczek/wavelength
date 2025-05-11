#ifndef WAVELENGTH_CONFIG_H
#define WAVELENGTH_CONFIG_H

#include <QColor>
#include <QKeySequence>
#include <QSettings>
#include <QString>

/** @brief Default configuration values for the Wavelength application. */
namespace DefaultConfig {
    const QString kRelayServerAddress = "localhost"; ///< Default relay server address
    constexpr int kRelayServerPort = 3000; ///< Default relay server port
    constexpr int kConnectionTimeout = 5000; ///< Default connection timeout in milliseconds
    constexpr int kKeepAliveInterval = 30000; ///< Default keep-alive interval in milliseconds
    constexpr int kMaxReconnectAttempts = 5; ///< Default maximum number of reconnection attempts
    constexpr bool kIsDebugMode = false; ///< Default debug mode status
    const auto kBackgroundColor = QColor(0x101820); ///< Dark Blue background color
    const auto kBlobColor = QColor(0x4682B4); ///< Steel Blue color for blob animation
    const auto kMessageColor = QColor(0xE0E0E0); ///< Light Gray color for messages
    const auto kStreamColor = QColor(0x00B3FF); ///< Light Blue color for stream visualization
    constexpr auto kGridColor = QColor(40, 60, 80, 150); ///< Grid color with transparency
    constexpr int kGridSpacing = 35; ///< Default grid spacing in pixels
    const auto kTitleTextColor = QColor(0xFFFFFF); ///< White color for title text
    const auto kTitleBorderColor = QColor(0xE0B0FF); ///< Light Purple color for a title border
    const auto kTitleGlowColor = QColor(0xE0B0FF); ///< Light Purple color for a title glow
}

/**
 * @brief Manages application configuration settings using a singleton pattern.
 *
 * This class handles loading, saving, and accessing various configuration parameters
 * for the Wavelength application, such as network settings, appearance colors,
 * application behavior limits, and keyboard shortcuts. It uses QSettings for persistence.
 */
class WavelengthConfig final : public QObject {
    Q_OBJECT

    /**
     * @brief Maximum number of recently used colors to store.
     */
    static constexpr int kMaxRecentColors = 5;

    /**
     * @brief Prefix used for storing shortcut keys within QSettings.
     */
    const QString kShortcutsPrefix = "Shortcuts/";

public:
    /**
     * @brief Gets the singleton instance of the WavelengthConfig.
     * @return Pointer to the singleton WavelengthConfig instance.
     */
    static WavelengthConfig *GetInstance();

    /**
     * @brief Default destructor.
     */
    ~WavelengthConfig() override = default;

    /**
     * @brief Gets the configured relay server address.
     * @return The relay server address as a QString.
     */
    QString GetRelayServerAddress() const;

    /**
     * @brief Sets the relay server address.
     * Emits configChanged("relayServerAddress") if the value changes.
     * @param address The new relay server address.
     */
    void SetRelayServerAddress(const QString &address);

    /**
     * @brief Gets the configured relay server port.
     * @return The relay server port number.
     */
    int GetRelayServerPort() const;

    /**
     * @brief Sets the relay server port.
     * Emits configChanged("relayServerPort") if the value changes.
     * @param port The new relay server port.
     */
    void SetRelayServerPort(int port);

    /**
     * @brief Constructs the full WebSocket URL for the relay server.
     * @return The URL as a QString (e.g., "ws://127.0.0.1:8080").
     */
    QString GetRelayServerUrl() const;

    /**
     * @brief Gets the connection timeout in milliseconds.
     * @return The connection timeout duration.
     */
    int GetConnectionTimeout() const;

    /**
     * @brief Sets the connection timeout in milliseconds.
     * Emits configChanged("connectionTimeout") if the value changes and is positive.
     * @param timeout The new connection timeout.
     */
    void SetConnectionTimeout(int timeout);

    /**
     * @brief Gets the keep-alive interval in milliseconds.
     * @return The keep-alive interval duration.
     */
    int GetKeepAliveInterval() const;

    /**
     * @brief Sets the keep-alive interval in milliseconds.
     * Emits configChanged("keepAliveInterval") if the value changes and is positive.
     * @param interval The new keep-alive interval.
     */
    void SetKeepAliveInterval(int interval);

    /**
     * @brief Gets the maximum number of reconnection attempts.
     * @return The maximum number of attempts.
     */
    int GetMaxReconnectAttempts() const;

    /**
     * @brief Sets the maximum number of reconnection attempts.
     * Emits configChanged("maxReconnectAttempts") if the value changes and is non-negative.
     * @param attempts The new maximum number of attempts.
     */
    void SetMaxReconnectAttempts(int attempts);

    /**
     * @brief Enables or disables debug mode.
     * Emits configChanged("debugMode") if the value changes.
     * @param enabled True to enable debug mode, false to disable.
     */
    void SetDebugMode(bool enabled);

    /**
    * @brief Checks if debug mode is enabled.
    * @return True if debug mode is enabled, false otherwise.
    */
    bool IsDebugMode() const;

    /**
     * @brief Gets the preferred starting frequency for new Wavelengths.
     * @return The preferred frequency as a string (e.g., "130.0").
     */
    QString GetPreferredStartFrequency() const;

    /**
     * @brief Sets the preferred starting frequency.
     * Validates that the frequency is a number >= 130.0. Normalizes to one decimal place.
     * Emits configChanged("preferredStartFrequency") if the normalized value changes.
     * @param frequency The desired frequency as a string.
     */
    void SetPreferredStartFrequency(const QString &frequency);

    /**
     * @brief Gets the main background color.
     * @return The background QColor.
     */
    QColor GetBackgroundColor() const;

    /**
     * @brief Sets the main background color.
     * Adds the color to recent colors.
     * Emits configChanged("background_color") if the value changes and is valid.
     * @param color The new background color.
     */
    void SetBackgroundColor(const QColor &color);

    /**
     * @brief Gets the color used for the blob animation.
     * @return The blob QColor.
     */
    QColor GetBlobColor() const;

    /**
     * @brief Sets the color for the blob animation.
     * Adds the color to recent colors.
     * Emits configChanged("blob_color") if the value changes and is valid.
     * @param color The new blob color.
     */
    void SetBlobColor(const QColor &color);

    /**
     * @brief Gets the color used for stream visualization elements.
     * @return The stream QColor.
     */
    QColor GetStreamColor() const;

    /**
     * @brief Sets the color for stream visualization elements.
     * Adds the color to recent colors.
     * Emits configChanged("stream_color") if the value changes and is valid.
     * @param color The new stream color.
     */
    void SetStreamColor(const QColor &color);

    /**
     * @brief Gets the color for the background grid lines.
     * @return The grid QColor.
     */
    QColor GetGridColor() const;

    /**
     * @brief Sets the color for the background grid lines.
     * Adds the color to recent colors.
     * Emits configChanged("grid_color") if the value changes and is valid.
     * @param color The new grid color.
     */
    void SetGridColor(const QColor &color);

    /**
     * @brief Gets the spacing between background grid lines in pixels.
     * @return The grid spacing.
     */
    int GetGridSpacing() const;

    /**
     * @brief Sets the spacing between background grid lines.
     * Emits configChanged("grid_spacing") if the value changes and is positive.
     * @param spacing The new grid spacing in pixels.
     */
    void SetGridSpacing(int spacing);

    /**
     * @brief Gets the color for the title label text.
     * @return The title text QColor.
     */
    QColor GetTitleTextColor() const;

    /**
     * @brief Sets the color for the title label text.
     * Adds the color to recent colors.
     * Emits configChanged("title_text_color") if the value changes and is valid.
     * @param color The new title text color.
     */
    void SetTitleTextColor(const QColor &color);

    /**
     * @brief Gets the color for the title label border.
     * @return The title border QColor.
     */
    QColor GetTitleBorderColor() const;

    /**
     * @brief Sets the color for the title label border.
     * Adds the color to recent colors.
     * Emits configChanged("title_border_color") if the value changes and is valid.
     * @param color The new title border color.
     */
    void SetTitleBorderColor(const QColor &color);

    /**
     * @brief Gets the color for the title label glow effect.
     * @return The title glow QColor.
     */
    QColor GetTitleGlowColor() const;

    /**
     * @brief Sets the color for the title label glow effect.
     * Adds the color to recent colors.
     * Emits configChanged("title_glow_color") if the value changes and is valid.
     * @param color The new title glow color.
     */
    void SetTitleGlowColor(const QColor &color);

    /**
     * @brief Gets the list of recently used colors.
     * @return A QStringList containing color hex codes (e.g., "#RRGGBB").
     */
    QStringList GetRecentColors() const;

    /**
     * @brief Adds a color to the list of recently used colors.
     * If the color already exists, it's moved to the front. The list is capped at kMaxRecentColors.
     * Emits recentColorsChanged() if the list is modified.
     * @param color The QColor to add.
     */
    void AddRecentColor(const QColor &color);

    /**
     * @brief Gets the configured keyboard shortcut for a specific action.
     * If no shortcut is configured for the action_id, it returns the default_sequence.
     * If default_sequence is empty, it looks up the default in the internal default map.
     * @param action_id The identifier of the action (e.g., "MainWindow.OpenSettings").
     * @param default_sequence An optional fallback sequence if the action_id is not found.
     * @return The QKeySequence for the action.
     */
    QKeySequence GetShortcut(const QString &action_id, const QKeySequence &default_sequence = QKeySequence()) const;

    /**
     * @brief Sets the keyboard shortcut for a specific action.
     * This updates the internal map; SaveSettings() must be called to persist the change.
     * @param action_id The identifier of the action.
     * @param sequence The new QKeySequence.
     */
    void SetShortcut(const QString &action_id, const QKeySequence &sequence);

    /**
     * @brief Gets a map of all currently configured shortcuts.
     * @return A QMap where keys are action IDs and values are QKeySequences.
     */
    QMap<QString, QKeySequence> GetAllShortcuts() const;

    /**
     * @brief Gets a map of the default shortcuts defined within the application.
     * @return A QMap containing the default action IDs and their corresponding QKeySequences.
     */
    QMap<QString, QKeySequence> GetDefaultShortcutsMap() const;

    /**
     * @brief Saves all current configuration settings to persistent storage (QSettings).
     */
    void SaveSettings();

    /**
     * @brief Restores all configuration settings to their default values.
     * Also clears saved shortcuts in persistent storage and saves the defaults.
     * Emits configChanged("all") and recentColorsChanged().
     */
    void RestoreDefaults();

    /**
     * @brief Gets a configuration setting value by its key.
     * This provides a generic way to access settings, primarily for UI elements.
     * @param key The string key identifying the setting (e.g., "relayServerPort", "backgroundColor").
     * @return A QVariant containing the setting's value, or an invalid QVariant if the key is unknown.
     */
    QVariant GetSetting(const QString &key) const;

    /**
     * @brief Gets the currently configured language code (e.g., "en", "pl").
     * @return The language code as a QString.
     */
    QString GetLanguageCode() const;

    /**
     * @brief Sets the application language code.
     * Emits configChanged("languageCode") if the value changes.
     * Note: Requires application restart to take full effect.
     * @param code The new language code (e.g., "en", "pl").
     */
    void SetLanguageCode(const QString &code);

signals:
    /**
     * @brief Emitted when a configuration setting changes.
     * @param key The key of the setting that changed (e.g., "blobColor", "maxChatHistorySize").
     *            Can be "all" if RestoreDefaults() was called.
     */
    void configChanged(const QString &key);

    /**
     * @brief Emitted when the list of recent colors changes (a color was added).
     */
    void recentColorsChanged();

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * Loads default shortcuts, default values, and then saved settings.
     * @param parent Optional parent QObject.
     */
    explicit WavelengthConfig(QObject *parent = nullptr);

    /**
     * @brief Deleted copy constructor.
     */
    WavelengthConfig(const WavelengthConfig &) = delete;

    /**
     * @brief Deleted assignment operator.
     */
    WavelengthConfig &operator=(const WavelengthConfig &) = delete;

    /**
     * @brief Loads the hardcoded default values for all settings into the member variables.
     * Also copies the default shortcuts map to the current shortcuts map.
     */
    void LoadDefaults();

    /**
     * @brief Loads settings from the persistent storage (QSettings) into the member variables,
     * overwriting the defaults if values exist in storage.
     */
    void LoadSettings();

    /**
     * @brief Loads the hardcoded default keyboard shortcuts into the default_shortcuts_ map.
     */
    void LoadDefaultShortcuts();

    /**
     * @brief Static pointer to the singleton instance.
     */
    static WavelengthConfig *instance_;

    /**
     * @brief QSettings object used for reading and writing configuration data.
     */
    QSettings settings_;

    /**
     * @brief Map storing the default keyboard shortcuts.
     * Key: Action ID (e.g., "MainWindow.OpenSettings"). Value: Default QKeySequence.
     */
    QMap<QString, QKeySequence> default_shortcuts_;

    /**
     * @brief Map storing the currently active keyboard shortcuts (loaded from settings or defaults).
     * Key: Action ID. Value: Current QKeySequence.
     */
    QMap<QString, QKeySequence> shortcuts_;
    QString relay_server_address_; ///< The address of the relay server
    int relay_server_port_{}; ///< The port of the relay server
    int connection_timeout_{}; ///< The connection timeout in milliseconds
    int keep_alive_interval_{}; ///< The keep-alive interval in milliseconds
    int max_reconnect_attempts_{}; ///< The maximum number of reconnection attempts
    bool debug_mode_{}; ///< Flag indicating if debug mode is enabled
    QColor background_color_; ///< The main background color of the application
    QColor blob_color_; ///< The color used for the blob animation
    QColor stream_color_; ///< The color used for stream visualization elements
    QStringList recent_colors_; ///< List of recently used colors (hex codes)
    QColor grid_color_; ///< The color for the background grid lines
    int grid_spacing_{}; ///< The spacing between background grid lines in pixels
    QColor title_text_color_; ///< The color for the title label text
    QColor title_border_color_; ///< The color for the title label border
    QColor title_glow_color_; ///< The color for the title label glow effect
    QString preferred_start_frequency_; ///< The preferred starting frequency for new Wavelengths
    QString language_code_; ///< The selected language code (e.g., "en", "pl")
};

#endif // WAVELENGTH_CONFIG_H
