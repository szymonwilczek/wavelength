#ifndef SHORTCUTS_SETTINGS_WIDGET_H
#define SHORTCUTS_SETTINGS_WIDGET_H

#include <QWidget>
#include <QMap>

class TranslationManager;
class WavelengthConfig;
class QFormLayout;
class QKeySequenceEdit;
class QLabel;
class QPushButton;

/**
 * @brief A widget for configuring keyboard shortcuts for various application actions.
 *
 * This widget displays a list of available actions and their currently assigned
 * keyboard shortcuts using QKeySequenceEdit widgets. Users can modify these shortcuts.
 * It interacts with the WavelengthConfig singleton to load the current shortcuts and
 * save the modified ones. A button is provided to restore all shortcuts to their
 * default values. Changes require an application restart to take effect.
 */
class ShortcutsSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs the ShortcutsSettingsWidget.
     * Initializes the UI, loads current shortcut settings from WavelengthConfig.
     * @param parent Optional parent widget.
     */
    explicit ShortcutsSettingsWidget(QWidget *parent = nullptr);

    /**
     * @brief Default destructor.
     */
    ~ShortcutsSettingsWidget() override = default;

    /**
     * @brief Loads the current shortcut settings from WavelengthConfig and updates the UI elements (QKeySequenceEdit widgets).
     */
    void LoadSettings() const;

    /**
     * @brief Saves the currently configured shortcuts from the UI (QKeySequenceEdit widgets) back to WavelengthConfig.
     * Note: Changes are saved to the config object but require an application restart to become active.
     */
    void SaveSettings() const;

private slots:
    /**
     * @brief Restores all keyboard shortcuts to their default values defined in WavelengthConfig.
     * Prompts the user for confirmation before proceeding. Updates the UI to reflect the restored defaults.
     */
    void RestoreDefaultShortcuts();

private:
    /**
     * @brief Creates and arranges all the UI elements (labels, input fields, layouts, button) for the widget.
     * Dynamically creates QKeySequenceEdit widgets based on the actions defined in WavelengthConfig.
     */
    void SetupUi();

    /**
     * @brief Static utility function to get a user-friendly description for a given shortcut action ID.
     * @param action_id The internal identifier for the shortcut action (e.g., "MainWindow.CreateWavelength").
     * @return A human-readable string describing the action, or an empty string if the ID is unknown.
     */
    static QString GetActionDescription(const QString &action_id);

    /** @brief Pointer to the WavelengthConfig singleton instance. */
    WavelengthConfig *config_;
    /** @brief Layout used to arrange the action labels and shortcut edit widgets. */
    QFormLayout *form_layout_;
    /** @brief Button to trigger the restoration of default shortcuts. */
    QPushButton *restore_button_;
    /** @brief Map storing pointers to the QKeySequenceEdit widgets, keyed by their corresponding action ID. */
    QMap<QString, QKeySequenceEdit *> shortcut_edits_;
    /** @brief Pointer to the TranslationManager for handling UI translations. */
    TranslationManager *translator_;
};

#endif // SHORTCUTS_SETTINGS_WIDGET_H
