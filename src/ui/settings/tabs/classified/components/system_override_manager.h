#ifndef SYSTEM_OVERRIDE_MANAGER_H
#define SYSTEM_OVERRIDE_MANAGER_H

#include <QAudioOutput>
#include <QObject>
#include <QTimer>
#include <QString>
#include <QSystemTrayIcon>
#include <QMediaPlayer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class FloatingEnergySphereWidget;

/**
 * @brief Manages a "system override" sequence involving visual and system-level changes.
 *
 * This class orchestrates a sequence that:
 * - Changes the desktop wallpaper to black (saving the original).
 * - Minimizes all open windows.
 * - Displays a system notification.
 * - Shows a full-screen, interactive FloatingEnergySphereWidget.
 * - Optionally plays background audio.
 * - On Windows, installs low-level keyboard and mouse hooks to block most input,
 *   allowing only specific keys needed for an Easter egg within the floating widget.
 *
 * It handles restoring the system state (wallpaper, unblocking input) when the sequence
 * finishes or is interrupted. It also includes logic for checking admin privileges and
 * relaunching the application with or without elevation on Windows.
 */
class SystemOverrideManager final : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a SystemOverrideManager.
     * Initializes member variables, sets up the system tray icon for notifications,
     * and initializes the media player. Performs COM initialization on Windows.
     * @param parent Optional parent QObject.
     */
    explicit SystemOverrideManager(QObject *parent = nullptr);

    /**
     * @brief Destructor.
     * Ensures the system state is restored if the override is active. Cleans up
     * the floating widget, temporary wallpaper file, and uninstalls hooks on Windows.
     * Performs COM uninitialization on Windows.
     */
    ~SystemOverrideManager() override;

#ifdef Q_OS_WIN
    /**
     * @brief Checks if the current process is running with administrative privileges (Windows only).
     * @return True if running as administrator, false otherwise.
     */
    static bool IsRunningAsAdmin();

    /**
     * @brief Attempts to relaunch the application with administrative privileges (Windows only).
     * Displays a UAC prompt to the user.
     * @param arguments Optional command-line arguments to pass to the relaunched instance.
     * @return True if the relaunch request was successfully initiated, false otherwise (e.g., user cancelled UAC).
     */
    static bool RelaunchAsAdmin(const QStringList& arguments = QStringList());
#endif

signals:
    /**
     * @brief Emitted when the system override sequence has finished and the system state has been restored.
     */
    void overrideFinished();

public slots:
    /**
     * @brief Initiates the full system override sequence.
     * Installs input hooks (Windows), changes wallpaper, minimizes windows, sends a notification,
     * and shows the floating animation widget.
     * @param is_first_time Flag indicating if this is the first time the sequence is run (affects floating widget's audio).
     */
    void InitiateOverrideSequence(bool is_first_time);

    /**
     * @brief Restores the system state to normal.
     * Uninstalls input hooks (Windows), restores the original wallpaper, removes the temporary
     * wallpaper file, closes the floating widget, and emits overrideFinished().
     * On Windows, attempts to relaunch the application normally and quit the elevated instance.
     */
    void RestoreSystemState();

private slots:
    /**
     * @brief Slot called when the FloatingEnergySphereWidget emits its widgetClosed signal.
     * Cleans up the widget pointer and triggers system state restoration after a delay.
     */
    void HandleFloatingWidgetClosed();

private:
    /**
     * @brief Saves the current wallpaper path and sets the desktop wallpaper to a temporary black image.
     * @return True on success, false otherwise.
     */
    bool ChangeWallpaper();

    /**
     * @brief Restores the original desktop wallpaper saved by ChangeWallpaper().
     * Also removes the temporary black wallpaper file.
     * @return True if the original wallpaper was successfully restored, false otherwise.
     */
    bool RestoreWallpaper();

    /**
     * @brief Sends a system notification using QSystemTrayIcon.
     * Ensures the tray icon is visible before sending.
     * @param title The title of the notification.
     * @param message The main text content of the notification.
     * @return True if the notification was sent successfully, false otherwise.
     */
    bool SendWindowsNotification(const QString& title, const QString& message) const;

    /**
     * @brief Minimizes all open windows on the desktop (Windows only).
     * Uses COM or a fallback method.
     * @return True on success, false otherwise.
     */
    static bool MinimizeAllWindows();

    /**
     * @brief Creates, configures, positions, and shows the FloatingEnergySphereWidget.
     * Connects necessary signals for closing and sequence completion.
     * @param is_first_time Flag passed to the FloatingEnergySphereWidget constructor.
     */
    void ShowFloatingAnimationWidget(bool is_first_time);

    /** @brief System tray icon used for displaying notifications. */
    QSystemTrayIcon* tray_icon_;
    /** @brief Pointer to the floating energy sphere widget instance. */
    FloatingEnergySphereWidget* floating_widget_;

    /** @brief Media player potentially used for background audio (currently not fully utilized). */
    QMediaPlayer* media_player_;
    /** @brief Audio output potentially used with media_player_ (currently not fully utilized). */
    QAudioOutput* audio_output_;

    /** @brief Stores the path to the user's original desktop wallpaper. */
    QString original_wallpaper_path_;
    /** @brief Stores the path to the temporary black wallpaper image file. */
    QString temp_black_wallpaper_path_;
    /** @brief Flag indicating if the system override sequence is currently active. */
    bool override_active_;

#ifdef Q_OS_WIN
    /**
     * @brief Attempts to relaunch the application without administrative privileges (Windows only).
     * Used after the override sequence to return to a normal instance.
     * @param arguments Optional command-line arguments to pass to the relaunched instance.
     * @return True if the relaunch was successfully initiated, false otherwise.
     */
    static bool RelaunchNormally(const QStringList& arguments = QStringList());

    // --- Hooks ---
    /**
     * @brief Low-level keyboard hook procedure (Windows only).
     * Filters keyboard input, allowing only specific keys defined in kAllowedKeys.
     * Blocks all other key presses when the override is active.
     * @param n_code Hook code.
     * @param w_param Type of keyboard event (e.g., WM_KEYDOWN).
     * @param l_param Pointer to KBDLLHOOKSTRUCT containing event details.
     * @return 1 to block the event, or CallNextHookEx result to pass it on.
     */
    static LRESULT CALLBACK LowLevelKeyboardProc(int n_code, WPARAM w_param, LPARAM l_param);

    /**
     * @brief Installs the global low-level keyboard hook (Windows only).
     * @return True if the hook was installed successfully, false otherwise.
     */
    static bool InstallKeyboardHook();

    /**
     * @brief Uninstalls the global low-level keyboard hook (Windows only).
     */
    static void UninstallKeyboardHook();
    /** @brief Handle to the installed low-level keyboard hook (Windows only). */
    static HHOOK keyboard_hook_;

    /**
     * @brief Low-level mouse hook procedure (Windows only).
     * Blocks all mouse input events (clicks, movement, wheel) when the override is active.
     * @param n_code Hook code.
     * @param w_param Type of mouse event (e.g., WM_LBUTTONDOWN).
     * @param l_param Pointer to MSLLHOOKSTRUCT containing event details.
     * @return 1 to block the event, or CallNextHookEx result to pass it on.
     */
    static LRESULT CALLBACK LowLevelMouseProc(int n_code, WPARAM w_param, LPARAM l_param);

    /**
     * @brief Installs the global low-level mouse hook (Windows only).
     * @return True if the hook was installed successfully, false otherwise.
     */
    static bool InstallMouseHook();

    /**
     * @brief Uninstalls the global low-level mouse hook (Windows only).
     */
    static void UninstallMouseHook();
    /** @brief Handle to the installed low-level mouse hook (Windows only). */
    static HHOOK mouse_hook_;
    // --------------------------------------------
#endif
};

#endif // SYSTEM_OVERRIDE_MANAGER_H