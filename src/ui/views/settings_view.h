#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H

#include <QStackedWidget>

class TranslationManager;
class SystemOverrideManager;
class QPushButton;
class SnakeGameLayer;
class TypingTestLayer;
class VoiceRecognitionLayer;
class RetinaScanLayer;
class SecurityQuestionLayer;
class SecurityCodeLayer;
class HandprintLayer;
class FingerprintLayer;
class CyberButton;
class ShortcutsSettingsWidget;
class NetworkSettingsWidget;
class AppearanceSettingsWidget;
class WavelengthSettingsWidget;
class TabButton;
class QLabel;
class WavelengthConfig;

/**
 * @brief The main view for configuring application settings.
 *
 * This widget provides a tabbed interface for accessing different settings categories:
 * Wavelength, Appearance, Network, Shortcuts, and a special "CLASSIFIED" tab.
 * The CLASSIFIED tab requires passing through a sequence of security layers (mini-games/challenges)
 * to access hidden features, including a system override function.
 * It interacts with WavelengthConfig to load/save settings and provides buttons for
 * saving, restoring defaults, and returning to the main application view.
 */
class SettingsView final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs the SettingsView.
     * Initializes the UI, sets up all tabs including the security layers for the CLASSIFIED tab,
     * connects signals and slots for UI interaction, and starts a timer for updating the displayed time.
     * @param parent Optional parent widget.
     */
    explicit SettingsView(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops the refresh timer and cleans up the SystemOverrideManager.
     */
    ~SettingsView() override;

    /**
     * @brief Enables or disables the debug mode for the CLASSIFIED tab.
     * When enabled, the security layers are bypassed, granting immediate access to classified features.
     * @param enabled True to enable debug mode, false to disable.
     */
    void SetDebugMode(bool enabled);

signals:
    /**
     * @brief Emitted when the user clicks the "BACK" button, requesting a return to the previous view.
     */
    void backToMainView();

    /**
     * @brief Emitted when settings are saved or restored to defaults, indicating that other parts
     * of the application might need to update based on the new configuration.
     */
    void settingsChanged();

protected:
    /**
     * @brief Overridden show event handler.
     * Loads settings from the configuration and resets the CLASSIFIED tab's security layers
     * every time the view becomes visible.
     * @param event The show event.
     */
    void showEvent(QShowEvent *event) override;

private slots:
    /**
     * @brief Saves all current settings from the various tabs to the WavelengthConfig object
     * and persists them. Displays a confirmation message. Emits settingsChanged().
     */
    void SaveSettings();

    /**
     * @brief Restores all settings across all tabs to their default values.
     * Prompts the user for confirmation before proceeding. Reloads settings into the UI
     * and emits settingsChanged().
     */
    void RestoreDefaults();

    /**
     * @brief Switches the visible tab content based on the clicked tab button.
     * Updates the active state of tab buttons and sets the current index of the QStackedWidget.
     * Resets the CLASSIFIED tab if it's selected.
     * @param tab_index The index of the tab to switch to.
     */
    void SwitchToTab(int tab_index);

    /**
     * @brief Handles the click event of the "BACK" button.
     * Resets the CLASSIFIED tab's security layers and emits the backToMainView() signal.
     */
    void HandleBackButton();

private:
    /**
     * @brief Loads settings from the WavelengthConfig object into the UI elements of all tabs.
     * Delegates loading to the individual tab widgets (LoadSettings methods).
     */
    void LoadSettingsFromRegistry() const;

    /**
     * @brief Creates and arranges the main UI elements of the SettingsView.
     * Sets up the main layout, header panel, tab bar, stacked widget for tab content,
     * and the bottom action buttons panel.
     */
    void SetupUi();

    /**
     * @brief Creates and configures the "CLASSIFIED" tab.
     * Sets up the stacked widget containing all the security layers (Fingerprint, Handprint, etc.)
     * and the final "Access Granted" widget with the system override button. Connects signals
     * for layer completion to advance through the sequence.
     */
    void SetupClassifiedTab();

    /**
     * @brief Advances to the next security layer in the CLASSIFIED tab sequence.
     * Updates the current_layer_index_, sets the active widget in the security_layers_stack_,
     * and calls Initialize() on the newly activated layer.
     * @deprecated Logic is now mostly handled within the connect statements in SetupClassifiedTab.
     */
    void SetupNextSecurityLayer();

    /**
     * @brief Resets all security layers in the CLASSIFIED tab to their initial state.
     * Calls Reset() on each layer. If debug mode is enabled, it bypasses the layers and shows
     * the final classified features widget directly. Otherwise, it sets the first layer (Fingerprint)
     * as active and initializes it.
     */
    void ResetSecurityLayers();

    /**
     * @brief Creates the header panel containing the view title, session ID, and current time labels.
     */
    void CreateHeaderPanel();

    /** @brief Pointer to the WavelengthConfig singleton instance. */
    WavelengthConfig *config_;

    /** @brief Stacked widget holding the content widgets for each settings tab. */
    QStackedWidget *tab_content_;
    /** @brief Label displaying the main title "SYSTEM SETTINGS". */
    QLabel *title_label_;
    /** @brief Label displaying a randomly generated session ID. */
    QLabel *session_label_;
    /** @brief Label displaying the current time, updated periodically. */
    QLabel *time_label_;
    /** @brief Container widget for the horizontal row of tab buttons. */
    QWidget *tab_bar_;
    /** @brief List of buttons used for switching between settings tabs. */
    QList<TabButton *> tab_buttons_;

    /** @brief Widget for configuring Wavelength-specific settings (e.g., preferred frequency). */
    WavelengthSettingsWidget *wavelength_tab_widget_;
    /** @brief Widget for configuring appearance settings (colors, etc.). */
    AppearanceSettingsWidget *appearance_tab_widget_;
    /** @brief Widget for configuring network settings (relay server, timeouts). */
    NetworkSettingsWidget *advanced_tab_widget_;
    /** @brief Widget for configuring keyboard shortcuts. */
    ShortcutsSettingsWidget *shortcuts_tab_widget_{};

    /** @brief Button to save the current settings. */
    CyberButton *save_button_;
    /** @brief Button to restore default settings. */
    CyberButton *defaults_button_;
    /** @brief Button to navigate back to the previous view. */
    CyberButton *back_button_;

    /** @brief Timer used to periodically update the time_label_. */
    QTimer *refresh_timer_;

    /** @brief Security layer simulating fingerprint scanning. */
    FingerprintLayer *fingerprint_layer_;
    /** @brief Security layer simulating handprint scanning. */
    HandprintLayer *handprint_layer_;
    /** @brief Security layer requiring a 4-character code entry. */
    SecurityCodeLayer *security_code_layer_;
    /** @brief Security layer simulating a security question challenge. */
    SecurityQuestionLayer *security_question_layer_;
    /** @brief Security layer simulating a retina scan. */
    RetinaScanLayer *retina_scan_layer_;
    /** @brief Security layer simulating voice recognition. */
    VoiceRecognitionLayer *voice_recognition_layer_;
    /** @brief Security layer implementing a typing verification test. */
    TypingTestLayer *typing_test_layer_;
    /** @brief Security layer implementing a Snake game challenge. */
    SnakeGameLayer *snake_game_layer_;
    /** @brief Widget displayed after successfully completing all security layers. Contains classified features. */
    QWidget *access_granted_widget_;
    /** @brief Stacked widget holding all the security layers and the final access granted widget. */
    QStackedWidget *security_layers_stack_;

    /** @brief Enum defining the order and indices of the security layers within the stack. */
    enum SecurityLayerIndex {
        FingerprintIndex = 0,
        HandprintIndex,
        SecurityCodeIndex,
        SecurityQuestionIndex,
        RetinaScanIndex,
        VoiceRecognitionIndex,
        TypingTestIndex,
        SnakeGameIndex,
        AccessGrantedIndex
    };

    /** @brief Index of the currently active security layer in the CLASSIFIED tab sequence. */
    SecurityLayerIndex current_layer_index_;
    /** @brief Flag indicating if debug mode is enabled (bypasses security layers). */
    bool debug_mode_enabled_;

    /** @brief Widget containing the classified features (currently just the override button). Same as access_granted_widget_. */
    QWidget *classified_features_widget_;
    /** @brief Button within the classified features widget to initiate the system override sequence. */
    QPushButton *override_button_;
    /** @brief Manages the system override sequence (wallpaper change, window minimize, input blocking, etc.). */
    SystemOverrideManager *system_override_manager_;
    /** @brief Pointer to the TranslationManager singleton instance for UI translations. */
    TranslationManager *translator_;
};

#endif // SETTINGS_VIEW_H
