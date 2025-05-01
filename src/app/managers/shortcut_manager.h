#ifndef SHORTCUT_MANAGER_H
#define SHORTCUT_MANAGER_H

#include <QObject>
#include <QKeySequence>
#include <QMap>
#include <concepts>

class QWidget;
class QShortcut;
class WavelengthConfig;
class QMainWindow;
class WavelengthChatView;
class SettingsView;
class Navbar;

/**
 * @brief Manages application-wide keyboard shortcuts using a singleton pattern.
 *
 * This class is responsible for registering, managing, and updating keyboard shortcuts
 * for different parts of the application (e.g., main window, chat view, settings view).
 * It retrieves shortcut sequences from WavelengthConfig and connects them to specific actions
 * within the relevant widgets. It also allows for dynamic updating of shortcuts if the
 * configuration changes.
 */
class ShortcutManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the ShortcutManager.
     * @return Pointer to the singleton ShortcutManager instance.
     */
    static ShortcutManager* GetInstance();

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    ShortcutManager(const ShortcutManager&) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    ShortcutManager& operator=(const ShortcutManager&) = delete;

    /**
     * @brief Registers shortcuts specific to the provided parent widget.
     *
     * Identifies the type of the parent widget (QMainWindow, WavelengthChatView, SettingsView)
     * and calls the appropriate private registration method. Clears any previously registered
     * shortcuts for this widget before registering new ones.
     * @param parent The widget for which to register shortcuts. Must not be null.
     */
    void RegisterShortcuts(QWidget* parent);

public slots:
    /**
     * @brief Updates the key sequences for all currently registered shortcuts.
     *
     * Iterates through all registered widgets and their associated shortcuts,
     * retrieves the latest key sequence from WavelengthConfig for each action ID,
     * and updates the QShortcut object if the sequence has changed.
     */
    void updateRegisteredShortcuts();

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * Initializes the pointer to the WavelengthConfig singleton.
     * @param parent Optional parent QObject.
     */
    explicit ShortcutManager(QObject *parent = nullptr);

    /**
     * @brief Private default destructor.
     */
    ~ShortcutManager() override = default;

    /**
     * @brief Pointer to the WavelengthConfig singleton instance for retrieving shortcut sequences.
     */
    WavelengthConfig* config_;

    /**
     * @brief Stores all registered shortcuts, organized by parent widget.
     * The outer map's key is the parent QWidget*.
     * The inner map's key is the action ID (QString, e.g., "MainWindow.OpenSettings")
     * and the value is the corresponding QShortcut* object.
     */
    QMap<QWidget*, QMap<QString, QShortcut*>> registered_shortcuts_;

    /**
     * @brief Registers shortcuts specific to the main application window.
     * @param window Pointer to the QMainWindow instance.
     * @param navbar Pointer to the Navbar instance within the main window.
     */
    void RegisterMainWindowShortcuts(QMainWindow* window, Navbar* navbar);

    /**
     * @brief Registers shortcuts specific to the WavelengthChatView.
     * @param chat_view Pointer to the WavelengthChatView instance.
     */
    void RegisterChatViewShortcuts(WavelengthChatView* chat_view);

    /**
     * @brief Registers shortcuts specific to the SettingsView.
     * @param settings_view Pointer to the SettingsView instance.
     */
    void RegisterSettingsViewShortcuts(SettingsView* settings_view);

    /**
     * @brief Template helper function to create and connect a QShortcut.
     *
     * Retrieves the key sequence for the given action_id from WavelengthConfig,
     * creates a QShortcut associated with the parent widget, connects its activated()
     * signal to the provided lambda function, and stores the shortcut in the
     * registered_shortcuts_ map.
     * @tparam Func The type of the callable (lambda function) to connect to the shortcut's activated signal.
     * @param action_id The unique identifier for the action (used to look up the key sequence in config).
     * @param parent The widget to which the shortcut will be attached.
     * @param lambda The function or lambda to execute when the shortcut is activated.
     */
    template<typename Func>
    requires std::invocable<Func>
    void CreateAndConnectShortcut(const QString& action_id, QWidget* parent, Func lambda);
};

#endif // SHORTCUT_MANAGER_H