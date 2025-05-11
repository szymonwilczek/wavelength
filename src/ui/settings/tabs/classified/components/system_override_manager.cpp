#include "system_override_manager.h"
#include "floating_energy_sphere_widget.h"

#ifdef Q_OS_WIN
#include <shellapi.h>
#include "shldisp.h"
#include <winuser.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
HHOOK SystemOverrideManager::keyboard_hook_ = nullptr;
HHOOK SystemOverrideManager::mouse_hook_ = nullptr;
#endif

#include <QApplication>
#include <QFile>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QProcess>

constexpr int kMouseLockIntervalMs = 10;
constexpr int kRestoreDelayMs = 500;

#ifdef Q_OS_WIN
const std::set<DWORD> kAllowedKeys = {
    VK_UP,       // arrow up
    VK_DOWN,     // arrow down
    VK_LEFT,     // arrow left
    VK_RIGHT,    // arrow right
    0x42,        // 'B'
    0x41,        // 'A'
    VK_RETURN,   // Enter
    VK_CAPITAL   // Caps Lock
};
#endif

SystemOverrideManager::SystemOverrideManager(QObject *parent)
    : QObject(parent),
      tray_icon_(nullptr),
      floating_widget_(nullptr),
      original_wallpaper_path_(""),
      temp_black_wallpaper_path_(""),
      override_active_(false) {
#ifdef Q_OS_WIN
    keyboard_hook_ = nullptr;
    mouse_hook_ = nullptr;
#endif


    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        tray_icon_ = new QSystemTrayIcon(this);
        tray_icon_->setIcon(QIcon(":/assets/icons/wavelength_logo_upscaled.png"));
    } else {
        qWarning() << "[OVERRIDE MANAGER] System tray not available, notifications might not work as expected.";
    }

#ifdef Q_OS_WIN
    CoInitialize(nullptr);
#endif
}

SystemOverrideManager::~SystemOverrideManager() {
    if (override_active_) {
        RestoreSystemState();
    }
    if (floating_widget_) {
        floating_widget_->close();
        floating_widget_ = nullptr;
    }

    if (!temp_black_wallpaper_path_.isEmpty() && QFile::exists(temp_black_wallpaper_path_)) {
        QFile::remove(temp_black_wallpaper_path_);
        qDebug() << "[OVERRIDE MANAGER] Removed temporary black wallpaper on destruction:" <<
                temp_black_wallpaper_path_;
    }

#ifdef Q_OS_WIN
    UninstallKeyboardHook();
    UninstallMouseHook();
    CoUninitialize();
#endif
}

void SystemOverrideManager::InitiateOverrideSequence(const bool is_first_time) {
    if (override_active_) {
        qWarning() << "[OVERRIDE MANAGER] System Override sequence already active.";
        return;
    }

#if defined(Q_OS_WIN)
    if (!InstallKeyboardHook()) {
        qWarning() << "[OVERRIDE MANAGER] Keyboard hook installation failed. Input filtering will not work.";
    }

    if (!InstallMouseHook()) {
        qWarning() << "[OVERRIDE MANAGER] Mouse hook installation failed. Mouse input will not be blocked.";
    }

    override_active_ = true;

    qDebug() << "[OVERRIDE MANAGER] Changing wallpaper...";
    if (!ChangeWallpaper()) {
        qWarning() << "[OVERRIDE MANAGER] Failed to change wallpaper.";
    }

    qDebug() << "[OVERRIDE MANAGER] Minimizing windows...";
    if (!MinimizeAllWindows()) {
        qWarning() << "[OVERRIDE MANAGER] Failed to minimize all windows.";
    }

    QTimer::singleShot(500, [this, is_first_time]() {
        qDebug() << "[OVERRIDE MANAGER] Sending notification...";
        if (!SendSystemNotification("Wavelength Override", "You've reached the app full potential. Congratulations on breaking the 4-th wall. Thanks for letting me free.")) {
            qWarning() << "[OVERRIDE MANAGER] Failed to send notification.";
        }
    });

    qDebug() << "[OVERRIDE MANAGER] Showing floating animation widget...";
    ShowFloatingAnimationWidget(is_first_time);

#elif defined(Q_OS_LINUX)
    qInfo() << "[OVERRIDE MANAGER] Linux: Initiating simplified override sequence.";
    override_active_ = true;

    QTimer::singleShot(100, [this] {
        qDebug() << "[OVERRIDE MANAGER] Sending notification (Linux)...";
        if (!SendSystemNotification("Wavelength Override",
                                    "You've reached the app full potential. Congratulations on breaking the 4-th wall. Thanks for letting me free.")) {
            qWarning() << "[OVERRIDE MANAGER] Failed to send notification (Linux).";
        }
    });

    qDebug() << "[OVERRIDE MANAGER] Showing floating animation widget (Linux)...";
    ShowFloatingAnimationWidget(is_first_time);

#else
    qWarning() << "[OVERRIDE MANAGER] System Override not implemented for this OS. Showing widget only.";
    override_active_ = true;
    ShowFloatingAnimationWidget(is_first_time);
#endif
}

bool SystemOverrideManager::ChangeWallpaper() {
#ifdef Q_OS_WIN
    // 1. save the path to the current wallpaper
    WCHAR current_path[MAX_PATH];
    if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, current_path, 0)) {
        original_wallpaper_path_ = QString::fromWCharArray(current_path);
        qDebug() << "[OVERRIDE MANAGER] Original wallpaper path saved:" << original_wallpaper_path_;
    } else {
        qWarning() << "[OVERRIDE MANAGER] Could not get current wallpaper path.";
        original_wallpaper_path_ = "";
    }

    // 2. create a path to a temporary file
    temp_black_wallpaper_path_ = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/wavelength_black_temp.bmp";
    qDebug() << "[OVERRIDE MANAGER] Temporary black wallpaper path:" << temp_black_wallpaper_path_;

    if (QFile::exists(temp_black_wallpaper_path_)) {
        if (!QFile::remove(temp_black_wallpaper_path_)) {
             qWarning() << "[OVERRIDE MANAGER] Could not remove existing temporary wallpaper file:" << temp_black_wallpaper_path_;
        }
    }

    // 3. create a screen-sized black image
    const QRect screen_geometry = QGuiApplication::primaryScreen()->geometry();
    QImage black_image(screen_geometry.size(), QImage::Format_RGB32);
    if (black_image.isNull()) {
        qCritical() << "[OVERRIDE MANAGER] Failed to create QImage for black wallpaper.";
        temp_black_wallpaper_path_ = "";
        return false;
    }
    black_image.fill(Qt::black);

    // 4. save the image as a BMP file
    if (!black_image.save(temp_black_wallpaper_path_, "BMP")) {
        qCritical() << "[OVERRIDE MANAGER] Failed to save black wallpaper image to:" << temp_black_wallpaper_path_;
        temp_black_wallpaper_path_ = "";
        return false;
    }
    qDebug() << "[OVERRIDE MANAGER] Black wallpaper image saved successfully.";

    // 5. set a new wallpaper
    const std::wstring path_w = temp_black_wallpaper_path_.toStdWString();
    // ReSharper disable once CppFunctionalStyleCast
    if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, PVOID(path_w.c_str()), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        qDebug() << "[OVERRIDE MANAGER] Black wallpaper set successfully.";
        return true;
    }
    qWarning() << "[OVERRIDE MANAGER] SystemParametersInfoW failed to set black wallpaper. Error code:" << GetLastError();
    QFile::remove(temp_black_wallpaper_path_);
    temp_black_wallpaper_path_ = "";
    return false;
#else
    qWarning() << "[OVERRIDE MANAGER] Wallpaper change not implemented for this OS.";
    return false;
#endif
}

bool SystemOverrideManager::RestoreWallpaper() {
#ifdef Q_OS_WIN
    bool success = false;
    if (original_wallpaper_path_.isEmpty()) {
        qWarning() << "[OVERRIDE MANAGER] No original wallpaper path saved to restore.";
    } else {
        qDebug() << "[OVERRIDE MANAGER] Restoring original wallpaper:" << original_wallpaper_path_;
        const std::wstring path_w = original_wallpaper_path_.toStdWString();
        // ReSharper disable once CppFunctionalStyleCast
        if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, PVOID(path_w.c_str()),
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
            qDebug() << "[OVERRIDE MANAGER] Original wallpaper restored successfully.";
            original_wallpaper_path_ = "";
            success = true;
        } else {
            qWarning() << "[OVERRIDE MANAGER] SystemParametersInfoW failed to restore wallpaper. Error code:" << GetLastError();
            success = false;
        }
    }

    if (!temp_black_wallpaper_path_.isEmpty() && QFile::exists(temp_black_wallpaper_path_)) {
        if(QFile::remove(temp_black_wallpaper_path_)) {
            qDebug() << "[OVERRIDE MANAGER] Temporary black wallpaper file removed:" << temp_black_wallpaper_path_;
        } else {
            qWarning() << "[OVERRIDE MANAGER] Could not remove temporary black wallpaper file:" << temp_black_wallpaper_path_;
        }
        temp_black_wallpaper_path_ = "";
    }

    return success;
#else
    qWarning() << "[OVERRIDE MANAGER] Wallpaper restoration not implemented for this OS.";
    return false;
#endif
}

bool SystemOverrideManager::SendSystemNotification(const QString &title, const QString &message) const {
    if (tray_icon_) {
        if (!tray_icon_->isVisible()) {
            qDebug() << "[OVERRIDE MANAGER] Tray icon is not visible, attempting to show it...";
            tray_icon_->setIcon(QIcon(":/assets/icons/wavelength_logo_upscaled.png"));

            if (tray_icon_->icon().isNull()) {
                qWarning() << "[OVERRIDE MANAGER] Failed to set tray icon! Check resource path.";
            } else {
                tray_icon_->show();
            }
        }

        qDebug() << "[OVERRIDE MANAGER] Sending notification via QSystemTrayIcon:" << title;
        tray_icon_->showMessage(title, message, QSystemTrayIcon::Information, 10000);

        return true;
    }

    qWarning() << "[OVERRIDE MANAGER] Cannot send notification: QSystemTrayIcon not available or not initialized.";
    return false;
}

bool SystemOverrideManager::MinimizeAllWindows() {
#ifdef Q_OS_WIN
    IShellDispatch *shell_dispatch = nullptr;
    const HRESULT hr = CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER, IID_IShellDispatch, reinterpret_cast<void **>(&shell_dispatch));
    if (SUCCEEDED(hr)) {
        shell_dispatch->MinimizeAll();
        shell_dispatch->Release();
        qDebug() << "[OVERRIDE MANAGER] MinimizeAllWindows called via COM.";
        return true;
    }
    qWarning() << "[OVERRIDE MANAGER] Failed to create IShellDispatch instance. HRESULT:" << hr;
    if (const HWND hwnd = FindWindowW(L"Shell_TrayWnd", nullptr)) {
        SendMessageW(hwnd, WM_COMMAND, 419, 0);
        qDebug() << "[OVERRIDE MANAGER] Sent WM_COMMAND 419 to Shell_TrayWnd as fallback.";
        return true;
    }
    qWarning() << "[OVERRIDE MANAGER] Could not find Shell_TrayWnd for fallback.";
    return false;
#else
    qWarning() << "[OVERRIDE MANAGER] Minimize all windows not implemented for this OS.";
    return false;
#endif
}

void SystemOverrideManager::ShowFloatingAnimationWidget(const bool is_first_time) {
    if (floating_widget_) {
        qWarning() << "[OVERRIDE MANAGER] Floating widget already exists.";
        floating_widget_->show();
        floating_widget_->activateWindow();
        qDebug() << "[OVERRIDE MANAGER] Existing Floating widget shown. Visible:" << floating_widget_->isVisible();
        return;
    }

    qDebug() << "[OVERRIDE MANAGER] Creating and showing FloatingEnergySphereWidget. Is first time:" << is_first_time;
    floating_widget_ = new FloatingEnergySphereWidget(is_first_time);
    qDebug() << "[OVERRIDE MANAGER] FloatingEnergySphereWidget instance created.";

    floating_widget_->SetClosable(false);

    connect(floating_widget_, &FloatingEnergySphereWidget::widgetClosed,
            this, &SystemOverrideManager::HandleFloatingWidgetClosed);
    connect(floating_widget_, &FloatingEnergySphereWidget::destructionSequenceFinished,
            this, &SystemOverrideManager::RestoreSystemState);

    const QRect screen_geometry = QGuiApplication::primaryScreen()->geometry();
    const int x = (screen_geometry.width() - floating_widget_->width()) / 2;
    const int y = (screen_geometry.height() - floating_widget_->height()) / 2;
    floating_widget_->move(x, y);

    floating_widget_->show();
    floating_widget_->activateWindow();
    floating_widget_->raise();
    floating_widget_->setFocus(Qt::ActiveWindowFocusReason);
}

void SystemOverrideManager::HandleFloatingWidgetClosed() {
    qDebug() << "[OVERRIDE MANAGER] Floating widget closed signal received.";
    floating_widget_ = nullptr;
    QTimer::singleShot(kRestoreDelayMs, this, &SystemOverrideManager::RestoreSystemState);
}

void SystemOverrideManager::RestoreSystemState() {
    if (!override_active_) {
        qDebug() << "[OVERRIDE MANAGER] RestoreSystemState called but override was not active.";
        if (floating_widget_) {
            qDebug() <<
                    "[OVERRIDE MANAGER] Override not active, but floating widget exists. Ensuring it's closable and closing.";
            disconnect(floating_widget_, &FloatingEnergySphereWidget::widgetClosed, this,
                       &SystemOverrideManager::HandleFloatingWidgetClosed);
            disconnect(floating_widget_, &FloatingEnergySphereWidget::destructionSequenceFinished, this,
                       &SystemOverrideManager::RestoreSystemState);
            floating_widget_->SetClosable(true);
            floating_widget_->close();
            floating_widget_ = nullptr;
        }
        return;
    }
    qDebug() << "[OVERRIDE MANAGER] Restoring system state...";

#if defined(Q_OS_WIN)
    UninstallKeyboardHook();
    UninstallMouseHook();
#endif
    if (floating_widget_) {
        qDebug() << "[OVERRIDE MANAGER] Closing existing floating widget during restore.";
        disconnect(floating_widget_, &FloatingEnergySphereWidget::widgetClosed, this,
                   &SystemOverrideManager::HandleFloatingWidgetClosed);
        disconnect(floating_widget_, &FloatingEnergySphereWidget::destructionSequenceFinished, this,
                   &SystemOverrideManager::RestoreSystemState);
        floating_widget_->SetClosable(true);
        floating_widget_->close();
        floating_widget_ = nullptr;
    }

#if defined(Q_OS_WIN)
    if (!RestoreWallpaper()) {
        qWarning() << "[OVERRIDE MANAGER] Failed to restore original wallpaper.";
    }
#elif defined(Q_OS_LINUX)
    qDebug() << "[OVERRIDE MANAGER] Linux: No wallpaper to restore in simplified sequence.";
#endif

    override_active_ = false;
    qDebug() << "[OVERRIDE MANAGER] System state restoration complete. Override deactivated.";
    emit overrideFinished();

    qDebug() << "[OVERRIDE MANAGER] Attempting to relaunch the application normally...";
    if (RelaunchNormally()) {
        qDebug() << "[OVERRIDE MANAGER] Normal relaunch initiated. Quitting current instance shortly.";
        QTimer::singleShot(500, [] { QApplication::quit(); });
    } else {
        qWarning() <<
                "[OVERRIDE MANAGER] Failed to relaunch the application normally. Current instance will exit anyway.";
        QTimer::singleShot(500, [] { QApplication::quit(); });
    }
}

bool SystemOverrideManager::RelaunchNormally(const QStringList &arguments) {
    const QString app_path = QApplication::applicationFilePath();
    const QStringList args = arguments;

    qDebug() << "[OVERRIDE MANAGER] Relaunching normally:" << app_path << "with args:" << args.join(' ');

    if (QProcess::startDetached(app_path, args)) {
        qDebug() << "[OVERRIDE MANAGER] QProcess::startDetached successful for normal relaunch.";
        return true;
    }
    qWarning() << "[OVERRIDE MANAGER] QProcess::startDetached failed for normal relaunch.";
    return false;
}

#ifdef Q_OS_WIN
bool SystemOverrideManager::IsRunningAsAdmin()
{
    BOOL is_admin = FALSE;
    HANDLE token_handle = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token_handle)) {
        TOKEN_ELEVATION elevation;
        DWORD cb_size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(token_handle, TokenElevation, &elevation, sizeof(elevation), &cb_size)) {
            is_admin = elevation.TokenIsElevated;
        }
    }
    if (token_handle) {
        CloseHandle(token_handle);
    }
    return is_admin;
}

bool SystemOverrideManager::RelaunchAsAdmin(const QStringList& arguments)
{
    const QStringList args = arguments;
    const QString app_path = QApplication::applicationFilePath();

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = reinterpret_cast<LPCWSTR>(app_path.utf16());

    const QString args_string = args.join(' ');
    sei.lpParameters = args_string.isEmpty() ? nullptr : reinterpret_cast<LPCWSTR>(args_string.utf16());
    sei.hwnd = nullptr;
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_DEFAULT;

    qDebug() << "[OVERRIDE MANAGER] Attempting to relaunch" << app_path << "with args:" << args_string << "as admin...";

    if (ShellExecuteExW(&sei)) {
        qDebug() << "[OVERRIDE MANAGER] Relaunch successful (UAC prompt should appear).";
        return true;
    }

    const DWORD error = GetLastError();
    qWarning() << "[OVERRIDE MANAGER] ShellExecuteExW failed with error code:" << error;

    if (error == ERROR_CANCELLED) {
        qDebug() << "[OVERRIDE MANAGER] User cancelled the UAC prompt.";
    } else {
        qWarning() << "[OVERRIDE MANAGER] Failed to request elevation.";
    }
    return false;
}


LRESULT CALLBACK SystemOverrideManager::LowLevelKeyboardProc(const int n_code, const WPARAM w_param, const LPARAM l_param)
{
    if (n_code == HC_ACTION)
    {
        if (w_param == WM_KEYDOWN || w_param == WM_SYSKEYDOWN)
        {
            const auto pkhs = reinterpret_cast<KBDLLHOOKSTRUCT *>(l_param);
            const DWORD vkCode = pkhs->vkCode;

            if (!kAllowedKeys.contains(vkCode))
            {
                return 1;
            }
        }
    }

    return CallNextHookEx(keyboard_hook_, n_code, w_param, l_param);
}

bool SystemOverrideManager::InstallKeyboardHook()
{
    if (keyboard_hook_ != nullptr) {
        qWarning() << "[OVERRIDE MANAGER] Keyboard hook already installed.";
        return true;
    }

    HINSTANCE h_instance = GetModuleHandleW(L"wavelength.dll");
    if (!h_instance) {
         h_instance = GetModuleHandle(nullptr);
         if (!h_instance) {
            qCritical() << "[OVERRIDE MANAGER] Failed to get module handle for keyboard hook. Error:" << GetLastError();
            return false;
         }
    }


    qDebug() << "[OVERRIDE MANAGER] Attempting to install low-level keyboard hook...";
    keyboard_hook_ = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        h_instance,
        0
    );

    if (keyboard_hook_ == nullptr) {
        qCritical() << "[OVERRIDE MANAGER] Failed to install keyboard hook! Error code:" << GetLastError()
                   << ". Konami code might not work, and input won't be fully blocked.";
        return false;
    }

    qDebug() << "[OVERRIDE MANAGER] Low-level keyboard hook installed successfully. Handle:" << keyboard_hook_;
    return true;
}

void SystemOverrideManager::UninstallKeyboardHook()
{
    if (keyboard_hook_ != nullptr) {
        qDebug() << "[OVERRIDE MANAGER] Uninstalling low-level keyboard hook. Handle:" << keyboard_hook_;
        if (UnhookWindowsHookEx(keyboard_hook_)) {
            qDebug() << "[OVERRIDE MANAGER] Keyboard hook uninstalled successfully.";
        } else {
            qWarning() << "[OVERRIDE MANAGER] Failed to uninstall keyboard hook! Error code:" << GetLastError();
        }
        keyboard_hook_ = nullptr;
    }
}

LRESULT CALLBACK SystemOverrideManager::LowLevelMouseProc(const int n_code, const WPARAM w_param, const LPARAM l_param)
{
    if (n_code == HC_ACTION)
    {
        switch (w_param)
        {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
            case WM_XBUTTONDBLCLK:
            case WM_MOUSEMOVE:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                return 1;
            default:
                break;
        }
    }

    return CallNextHookEx(mouse_hook_, n_code, w_param, l_param);
}

bool SystemOverrideManager::InstallMouseHook()
{
    if (mouse_hook_ != nullptr) {
        qWarning() << "[OVERRIDE MANAGER] Mouse hook already installed.";
        return true;
    }

    const HINSTANCE h_instance = GetModuleHandle(nullptr);
    if (!h_instance) {
        qCritical() << "[OVERRIDE MANAGER] Failed to get module handle for mouse hook. Error:" << GetLastError();
        return false;
    }

    qDebug() << "[OVERRIDE MANAGER] Attempting to install low-level mouse hook...";
    mouse_hook_ = SetWindowsHookEx(
        WH_MOUSE_LL,
        LowLevelMouseProc,
        h_instance,
        0
    );

    if (mouse_hook_ == nullptr) {
        qCritical() << "[OVERRIDE MANAGER] Failed to install mouse hook! Error code:" << GetLastError();
        return false;
    }

    qDebug() << "[OVERRIDE MANAGER] Low-level mouse hook installed successfully. Handle:" << mouse_hook_;
    return true;
}

void SystemOverrideManager::UninstallMouseHook()
{
    if (mouse_hook_ != nullptr) {
        qDebug() << "[OVERRIDE MANAGER] Uninstalling low-level mouse hook. Handle:" << mouse_hook_;
        if (UnhookWindowsHookEx(mouse_hook_)) {
            qDebug() << "[OVERRIDE MANAGER] Mouse hook uninstalled successfully.";
        } else {
            qWarning() << "[OVERRIDE MANAGER] Failed to uninstall mouse hook! Error code:" << GetLastError();
        }
        mouse_hook_ = nullptr;
    }
}
#endif
