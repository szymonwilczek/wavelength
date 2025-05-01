#ifndef WAVELENGTH_CONFIG_H
#define WAVELENGTH_CONFIG_H

#include <QColor>
#include <QString>
#include <QSettings>
#include <QHostAddress>
#include <QMap>
#include <QVariant>
#include <QKeySequence>

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
    static WavelengthConfig* GetInstance();

    /**
     * @brief Default destructor.
     */
    ~WavelengthConfig() override = default;

    // --- Network Settings ---

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
    void SetRelayServerAddress(const QString& address);

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

    // --- Application Settings ---

    /**
     * @brief Gets the maximum number of messages to keep in the chat history.
     * @return The maximum chat history size.
     */
    int GetMaxChatHistorySize() const;

    /**
     * @brief Sets the maximum number of messages to keep in the chat history.
     * Emits configChanged("maxChatHistorySize") if the value changes and is positive.
     * @param size The new maximum size.
     */
    void SetMaxChatHistorySize(int size);

    /**
     * @brief Gets the maximum number of processed message IDs to store (for duplicate detection).
     * @return The maximum number of processed message IDs.
     */
    int GetMaxProcessedMessageIds() const;

    /**
     * @brief Sets the maximum number of processed message IDs to store.
     * Emits configChanged("maxProcessedMessageIds") if the value changes and is positive.
     * @param size The new maximum size.
     */
    void SetMaxProcessedMessageIds(int size);

    /**
     * @brief Gets the maximum number of sent messages to cache (for potential resending).
     * @return The maximum sent message cache size.
     */
    int GetMaxSentMessageCacheSize() const;

    /**
     * @brief Sets the maximum number of sent messages to cache.
     * Emits configChanged("maxSentMessageCacheSize") if the value changes and is positive.
     * @param size The new maximum size.
     */
    void SetMaxSentMessageCacheSize(int size);

    /**
     * @brief Gets the maximum number of recent Wavelength connections to remember.
     * @return The maximum number of recent Wavelengths.
     */
    int GetMaxRecentWavelength() const;

    /**
     * @brief Sets the maximum number of recent Wavelength connections to remember.
     * Emits configChanged("maxRecentWavelength") if the value changes and is positive.
     * @param size The new maximum size.
     */
    void SetMaxRecentWavelength(int size);

    /**
     * @brief Checks if debug mode is enabled.
     * @return True if debug mode is enabled, false otherwise.
     */
    bool IsDebugMode() const;

    /**
     * @brief Enables or disables debug mode.
     * Emits configChanged("debugMode") if the value changes.
     * @param enabled True to enable debug mode, false to disable.
     */
    void SetDebugMode(bool enabled);

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

    // --- Appearance Settings ---

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
    void SetBackgroundColor(const QColor& color);

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
    void SetBlobColor(const QColor& color);

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
    void SetStreamColor(const QColor& color);

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
    void SetGridColor(const QColor& color);

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
    void SetTitleTextColor(const QColor& color);

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
    void SetTitleBorderColor(const QColor& color);

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
    void SetTitleGlowColor(const QColor& color);

    // --- Recent Colors ---

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
    void AddRecentColor(const QColor& color);

    // --- Shortcuts ---

    /**
     * @brief Gets the configured keyboard shortcut for a specific action.
     * If no shortcut is configured for the action_id, it returns the default_sequence.
     * If default_sequence is empty, it looks up the default in the internal default map.
     * @param action_id The identifier of the action (e.g., "MainWindow.OpenSettings").
     * @param default_sequence An optional fallback sequence if the action_id is not found.
     * @return The QKeySequence for the action.
     */
    QKeySequence GetShortcut(const QString& action_id, const QKeySequence& default_sequence = QKeySequence()) const;

    /**
     * @brief Sets the keyboard shortcut for a specific action.
     * This updates the internal map; SaveSettings() must be called to persist the change.
     * @param action_id The identifier of the action.
     * @param sequence The new QKeySequence.
     */
    void SetShortcut(const QString& action_id, const QKeySequence& sequence);

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

    // --- Persistence ---

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

    // --- Generic Access ---

    /**
     * @brief Gets a configuration setting value by its key.
     * This provides a generic way to access settings, primarily for UI elements.
     * @param key The string key identifying the setting (e.g., "relayServerPort", "backgroundColor").
     * @return A QVariant containing the setting's value, or an invalid QVariant if the key is unknown.
     */
    QVariant GetSetting(const QString& key) const;

signals:
    /**
     * @brief Emitted when a configuration setting changes.
     * @param key The key of the setting that changed (e.g., "blobColor", "maxChatHistorySize").
     *            Can be "all" if RestoreDefaults() was called.
     */
    void configChanged(const QString& key);

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
    WavelengthConfig(const WavelengthConfig&) = delete;

    /**
     * @brief Deleted assignment operator.
     */
    WavelengthConfig& operator=(const WavelengthConfig&) = delete;

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
    static WavelengthConfig* instance_;

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

    // --- Member Variables holding current configuration values ---
    QString relay_server_address_;  ///< The address of the relay server
    int relay_server_port_;  ///< The port of the relay server
    int max_chat_history_size_; ///< The maximum number of messages to keep in chat history
    int max_processed_message_ids_; ///< The maximum number of processed message IDs to store
    int max_sent_message_cache_size_; ///< The maximum number of sent messages to cache
    int max_recent_wavelength_; ///< The maximum number of recent Wavelength connections to remember
    int connection_timeout_; ///< The connection timeout in milliseconds
    int keep_alive_interval_; ///< The keep-alive interval in milliseconds
    int max_reconnect_attempts_; ///< The maximum number of reconnection attempts
    bool debug_mode_; ///< Flag indicating if debug mode is enabled
    QColor background_color_; ///< The main background color of the application
    QColor blob_color_; ///< The color used for the blob animation
    QColor stream_color_; ///< The color used for stream visualization elements
    QStringList recent_colors_; ///< List of recently used colors (hex codes)
    QColor grid_color_; ///< The color for the background grid lines
    int grid_spacing_; ///< The spacing between background grid lines in pixels
    QColor title_text_color_; ///< The color for the title label text
    QColor title_border_color_; ///< The color for the title label border
    QColor title_glow_color_; ///< The color for the title label glow effect
    QString preferred_start_frequency_; ///< The preferred starting frequency for new Wavelengths
};

#endif // WAVELENGTH_CONFIG_H