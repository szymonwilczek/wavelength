#include "system_override_manager.h"
#include "floating_energy_sphere_widget.h"
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QDir>
#include <QMessageBox>
#include <qprocess.h>
#include <QStandardPaths>
#include <random>
#include <set>

#ifdef Q_OS_WIN
#include <shellapi.h>
#include "shldisp.h"
#include <winuser.h>
#endif

#ifdef Q_OS_WIN
// --- DEFINICJA STATYCZNEGO POLA ---
HHOOK SystemOverrideManager::keyboard_hook_ = nullptr;
HHOOK SystemOverrideManager::mouse_hook_ = nullptr;
// ---------------------------------
#endif

// --- Stałe ---
constexpr int kMouseLockIntervalMs = 10;
constexpr int kRestoreDelayMs = 500;

#ifdef Q_OS_WIN
const std::set<DWORD> kAllowedKeys = {
    VK_UP,       // Strzałka w górę
    VK_DOWN,     // Strzałka w dół
    VK_LEFT,     // Strzałka w lewo
    VK_RIGHT,    // Strzałka w prawo
    0x42,        // Klawisz 'B'
    0x41,        // Klawisz 'A'
    VK_RETURN,    // Klawisz Enter
    VK_CAPITAL // Klawisz Caps Lock
};
#endif

SystemOverrideManager::SystemOverrideManager(QObject *parent)
    : QObject(parent),
      // m_mouseTargetTimer nie jest już inicjalizowany
      tray_icon_(nullptr),
      floating_widget_(nullptr),
      media_player_(nullptr),
      audio_output_(nullptr),
      original_wallpaper_path_(""),
      temp_black_wallpaper_path_(""),
      override_active_(false)
{

#ifdef Q_OS_WIN
    // Upewnij się, że hak jest NULL na starcie
    keyboard_hook_ = nullptr;
    mouse_hook_ = nullptr;
#endif


    // Inicjalizacja powiadomień
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        tray_icon_ = new QSystemTrayIcon(this);
        // Można ustawić ikonę, jeśli jest potrzebna
        // m_trayIcon->setIcon(QIcon(":/assets/icons/wavelength_logo_upscaled.png"));
        // Nie pokazujemy ikony w zasobniku, tylko używamy do powiadomień
    } else {
        qWarning() << "System tray not available, notifications might not work as expected.";
    }

    // Inicjalizacja dźwięku
    media_player_ = new QMediaPlayer(this);
    // m_audioOutput = new QAudioOutput(this);
    // m_mediaPlayer->setAudioOutput(m_audioOutput);
    // m_mediaPlayer->setSource(QUrl(OVERRIDE_SOUND_RESOURCE));
    // m_audioOutput->setVolume(0.7); // Ustaw głośność (0.0 - 1.0)
    // m_mediaPlayer->setLoops(QMediaPlayer::Infinite); // Pętla dźwięku

#ifdef Q_OS_WIN
    CoInitialize(nullptr);
#endif
}

SystemOverrideManager::~SystemOverrideManager()
{
    if (override_active_) {
        RestoreSystemState();
    }
    if (floating_widget_) {
        floating_widget_->close();
        floating_widget_ = nullptr;
    }

    if (!temp_black_wallpaper_path_.isEmpty() && QFile::exists(temp_black_wallpaper_path_)) {
        QFile::remove(temp_black_wallpaper_path_);
        qDebug() << "Removed temporary black wallpaper on destruction:" << temp_black_wallpaper_path_;
    }

#ifdef Q_OS_WIN
    UninstallKeyboardHook();
    UninstallMouseHook();
    CoUninitialize();
#endif
}

void SystemOverrideManager::InitiateOverrideSequence(const bool is_first_time)
{
    if (override_active_) {
        qWarning() << "System Override sequence already active.";
        return;
    }

#ifdef Q_OS_WIN
    // --- Zainstaluj Hak Klawiatury ---
    if (!InstallKeyboardHook()) {
        // Co zrobić, jeśli hak się nie zainstaluje?
        // Można przerwać, wyświetlić ostrzeżenie lub kontynuować bez blokady klawiatury
        qWarning() << "Keyboard hook installation failed. Input filtering will not work.";
        // Można rozważyć przerwanie:
        // m_overrideActive = false;
        // emit overrideSequenceStartFailed("Failed to install keyboard input filter.");
        // return;
    }

    if (!InstallMouseHook()) { // <<< NOWE
        qWarning() << "Mouse hook installation failed. Mouse input will not be blocked.";
        // Rozważ przerwanie
    }
    // ---------------------------------
#endif

    override_active_ = true;

    // --- Kolejne kroki ---
    qDebug() << "Changing wallpaper...";
    if (!ChangeWallpaper()) {
        qWarning() << "Failed to change wallpaper.";
    }

    qDebug() << "Minimizing windows...";
    if (!MinimizeAllWindows()) {
        qWarning() << "Failed to minimize all windows.";
    }

    // Opóźnienie przed pokazaniem powiadomienia, aby użytkownik zdążył zobaczyć pulpit
    QTimer::singleShot(500, [this]() {
        qDebug() << "Sending notification...";
        if (!SendWindowsNotification("Wavelength Override", "You've reached the app full potential. Congratulations on breaking the 4-th wall. Thanks for letting me free.")) {
            qWarning() << "Failed to send notification.";
        }
    });

    qDebug() << "Showing floating animation widget...";
    ShowFloatingAnimationWidget(is_first_time); // Pokazuje widget i uruchamia dźwięk wewnątrz
}

bool SystemOverrideManager::ChangeWallpaper()
{
#ifdef Q_OS_WIN
    // 1. Zapisz ścieżkę do obecnej tapety
    WCHAR current_path[MAX_PATH];
    if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, current_path, 0)) {
        original_wallpaper_path_ = QString::fromWCharArray(current_path);
        qDebug() << "Original wallpaper path saved:" << original_wallpaper_path_;
    } else {
        qWarning() << "Could not get current wallpaper path.";
        original_wallpaper_path_ = "";
    }

    // 2. Utwórz ścieżkę do pliku tymczasowego
    temp_black_wallpaper_path_ = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/wavelength_black_temp.bmp";
    qDebug() << "Temporary black wallpaper path:" << temp_black_wallpaper_path_;

    // Usuń stary plik tymczasowy, jeśli istnieje
    if (QFile::exists(temp_black_wallpaper_path_)) {
        if (!QFile::remove(temp_black_wallpaper_path_)) {
             qWarning() << "Could not remove existing temporary wallpaper file:" << temp_black_wallpaper_path_;
             // Kontynuuj mimo wszystko, save() powinien nadpisać
        }
    }

    // 3. Utwórz czarny obraz o rozmiarze ekranu
    const QRect screen_geometry = QGuiApplication::primaryScreen()->geometry();
    QImage black_image(screen_geometry.size(), QImage::Format_RGB32);
    if (black_image.isNull()) {
        qCritical() << "Failed to create QImage for black wallpaper.";
        temp_black_wallpaper_path_ = ""; // Wyczyść ścieżkę w razie błędu
        return false;
    }
    black_image.fill(Qt::black);

    // 4. Zapisz obraz jako plik BMP
    if (!black_image.save(temp_black_wallpaper_path_, "BMP")) {
        qCritical() << "Failed to save black wallpaper image to:" << temp_black_wallpaper_path_;
        temp_black_wallpaper_path_ = ""; // Wyczyść ścieżkę w razie błędu
        return false;
    }
    qDebug() << "Black wallpaper image saved successfully.";

    // 5. Ustaw nową tapetę
    const std::wstring path_w = temp_black_wallpaper_path_.toStdWString();
    // ReSharper disable once CppFunctionalStyleCast
    if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, PVOID(path_w.c_str()), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        qDebug() << "Black wallpaper set successfully.";
        return true;
    }
    qWarning() << "SystemParametersInfoW failed to set black wallpaper. Error code:" << GetLastError();
    // Usuń plik tymczasowy w razie błędu ustawiania
    QFile::remove(temp_black_wallpaper_path_);
    temp_black_wallpaper_path_ = "";
    return false;
#else
    qWarning() << "Wallpaper change not implemented for this OS.";
    return false;
#endif
}

bool SystemOverrideManager::RestoreWallpaper()
{
#ifdef Q_OS_WIN
    bool success = false;
    if (original_wallpaper_path_.isEmpty()) {
        qWarning() << "No original wallpaper path saved to restore.";
        // Mimo to spróbuj usunąć czarną tapetę, jeśli istnieje
    } else {
        qDebug() << "Restoring original wallpaper:" << original_wallpaper_path_;
        const std::wstring path_w = original_wallpaper_path_.toStdWString();
        // ReSharper disable once CppFunctionalStyleCast
        if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, PVOID(path_w.c_str()),
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
            qDebug() << "Original wallpaper restored successfully.";
            original_wallpaper_path_ = ""; // Wyczyść po przywróceniu
            success = true; // Oznacz sukces przywracania
        } else {
            qWarning() << "SystemParametersInfoW failed to restore wallpaper. Error code:" << GetLastError();
            // Nie czyść m_originalWallpaperPath, może się udać później
            success = false; // Oznacz błąd przywracania
        }
    }

    // Zawsze próbuj usunąć tymczasowy plik czarnej tapety po próbie przywrócenia
    if (!temp_black_wallpaper_path_.isEmpty() && QFile::exists(temp_black_wallpaper_path_)) {
        if(QFile::remove(temp_black_wallpaper_path_)) {
            qDebug() << "Temporary black wallpaper file removed:" << temp_black_wallpaper_path_;
        } else {
            qWarning() << "Could not remove temporary black wallpaper file:" << temp_black_wallpaper_path_;
            // To nie jest krytyczne, ale warto zalogować
        }
        temp_black_wallpaper_path_ = ""; // Wyczyść ścieżkę niezależnie od sukcesu usunięcia
    }

    return success; // Zwróć sukces *przywracania* oryginalnej tapety
#else
    qWarning() << "Wallpaper restoration not implemented for this OS.";
    return false;
#endif
}

bool SystemOverrideManager::SendWindowsNotification(const QString& title, const QString& message) const {
    if (tray_icon_) {
        if (!tray_icon_->isVisible()) {
            qDebug() << "Tray icon is not visible, attempting to show it...";
            // --- USTAW IKONĘ TUTAJ ---
            // Użyj ikony z zasobów, jeśli masz odpowiednią
            // Pamiętaj, aby dodać ikonę do pliku .qrc
            tray_icon_->setIcon(QIcon(":/assets/icons/wavelength_logo_upscaled.png")); // Przykładowa ścieżka
            // -------------------------

            // Sprawdź, czy ikona została ustawiona
            if (tray_icon_->icon().isNull()) {
                qWarning() << "Failed to set tray icon! Check resource path.";
            } else {
                tray_icon_->show(); // Pokaż ikonę dopiero po jej ustawieniu
            }
        }

        qDebug() << "Sending notification via QSystemTrayIcon:" << title;
        tray_icon_->showMessage(title, message, QSystemTrayIcon::Information, 10000);

        // Opcjonalnie: ukryj ikonę po chwili
        // QTimer::singleShot(11000, this, [this](){ if(m_trayIcon && m_trayIcon->isVisible()) m_trayIcon->hide(); });

        return true;
    } else {
        qWarning() << "Cannot send notification: QSystemTrayIcon not available or not initialized.";
        return false;
    }
}

bool SystemOverrideManager::MinimizeAllWindows()
{
#ifdef Q_OS_WIN
    // Użycie COM jest bardziej niezawodne
    IShellDispatch *shell_dispatch = nullptr;
    const HRESULT hr = CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER, IID_IShellDispatch, reinterpret_cast<void **>(&shell_dispatch));
    if (SUCCEEDED(hr)) {
        shell_dispatch->MinimizeAll();
        shell_dispatch->Release();
        qDebug() << "MinimizeAllWindows called via COM.";
        return true;
    }
    qWarning() << "Failed to create IShellDispatch instance. HRESULT:" << hr;
    // Fallback na starszą metodę
    if (const HWND hwnd = FindWindowW(L"Shell_TrayWnd", nullptr)) {
        SendMessageW(hwnd, WM_COMMAND, 419, 0);
        qDebug() << "Sent WM_COMMAND 419 to Shell_TrayWnd as fallback.";
        return true;
    }
    qWarning() << "Could not find Shell_TrayWnd for fallback.";
    return false;
#else
    qWarning() << "Minimize all windows not implemented for this OS.";
    return false;
#endif
}

void SystemOverrideManager::ShowFloatingAnimationWidget(const bool is_first_time)
{
    if (floating_widget_) {
        qWarning() << "Floating widget already exists.";
        floating_widget_->show();
        floating_widget_->activateWindow();
        qDebug() << "Existing Floating widget shown. Visible:" << floating_widget_->isVisible();
        return;
    }

    qDebug() << "Creating and showing FloatingEnergySphereWidget. Is first time:" << is_first_time; // Dodaj log
    // Przekaż flagę isFirstTime do konstruktora widgetu
    floating_widget_ = new FloatingEnergySphereWidget(is_first_time); // <<< ZMIANA TUTAJ
    if (!floating_widget_) {
        qCritical() << "Failed to create FloatingEnergySphereWidget instance!";
        return;
    }
    qDebug() << "FloatingEnergySphereWidget instance created.";

    floating_widget_->SetClosable(false);

    connect(floating_widget_, &FloatingEnergySphereWidget::widgetClosed,
            this, &SystemOverrideManager::HandleFloatingWidgetClosed);

    // Zamiast konamiCodeEntered, połącz nowy sygnał zakończenia sekwencji niszczenia
    // connect(m_floatingWidget, &FloatingEnergySphereWidget::konamiCodeEntered,
    //         this, &SystemOverrideManager::restoreSystemState); // <<< STARE POŁĄCZENIE
    connect(floating_widget_, &FloatingEnergySphereWidget::destructionSequenceFinished,
            this, &SystemOverrideManager::RestoreSystemState); // <<< NOWE POŁĄCZENIE

    const QRect screen_geometry = QGuiApplication::primaryScreen()->geometry();
    const int x = (screen_geometry.width() - floating_widget_->width()) / 2;
    const int y = (screen_geometry.height() - floating_widget_->height()) / 2;
    floating_widget_->move(x, y);

    floating_widget_->show();
    floating_widget_->activateWindow();
    floating_widget_->setFocus();

}

void SystemOverrideManager::HandleFloatingWidgetClosed()
{
    qDebug() << "Floating widget closed signal received.";
    floating_widget_ = nullptr; // Widget sam się usuwa

    // Zatrzymaj dźwięk
    // if (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
    //     m_mediaPlayer->stop();
    // }

    // Rozpocznij przywracanie stanu (w tym odblokowanie wejścia)
    QTimer::singleShot(kRestoreDelayMs, this, &SystemOverrideManager::RestoreSystemState);
}

void SystemOverrideManager::RestoreSystemState()
{
    if (!override_active_) { /* ... */ return; }
    qDebug() << "Restoring system state...";

#ifdef Q_OS_WIN
    // --- Odinstaluj Haki ---
    UninstallKeyboardHook();
    UninstallMouseHook();
    // -----------------------
#endif

    // Zatrzymaj i zamknij widget animacji, jeśli nadal istnieje
    if (floating_widget_) {
        qDebug() << "Closing existing floating widget during restore.";
        // Rozłącz sygnały, aby uniknąć ponownego wywołania restoreSystemState
        disconnect(floating_widget_, &FloatingEnergySphereWidget::widgetClosed, this, &SystemOverrideManager::HandleFloatingWidgetClosed);
        disconnect(floating_widget_, &FloatingEnergySphereWidget::destructionSequenceFinished, this, &SystemOverrideManager::RestoreSystemState);
        floating_widget_->SetClosable(true);
        floating_widget_->close();
        floating_widget_ = nullptr;
    }

    // Przywróć tapetę
    if (!RestoreWallpaper()) {
        qWarning() << "Failed to restore original wallpaper.";
    }

    // Oznacz jako nieaktywny i wyemituj sygnał (jeśli był aktywny)
    if(override_active_) {
        override_active_ = false;
        qDebug() << "System state restoration complete. Override deactivated.";
        emit overrideFinished(); // Emituj sygnał *przed* próbą relaunchu
    } else {
        qDebug() << "System state restoration complete (input unblocked only).";
    }

    // --- NOWE: Relaunch normalnej instancji i zamknij bieżącą (podwyższoną) ---
    qDebug() << "Attempting to relaunch the application normally...";
#ifdef Q_OS_WIN
    if (RelaunchNormally()) { // Wywołaj nową funkcję pomocniczą
        qDebug() << "Normal relaunch initiated. Quitting elevated instance shortly.";
        // Użyj QTimer::singleShot, aby dać nowemu procesowi chwilę na start
        QTimer::singleShot(500, [](){ QApplication::quit(); });
    } else {
        qWarning() << "Failed to relaunch the application normally. Elevated instance will exit anyway.";
        QTimer::singleShot(500, [](){ QApplication::quit(); }); // Zamknij mimo wszystko
    }
#else
    // Na innych systemach po prostu zamknij
    qWarning() << "Normal relaunch not implemented for this OS. Quitting elevated instance.";
    QTimer::singleShot(100, [](){ QApplication::quit(); });
#endif
    // -----------------------------------------------------------------------
}

#ifdef Q_OS_WIN
// --- Implementacja funkcji pomocniczych ---

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
    sei.lpVerb = L"runas"; // Kluczowe: żądanie podwyższenia uprawnień
    sei.lpFile = reinterpret_cast<LPCWSTR>(app_path.utf16());
    // Konwertuj argumenty na pojedynczy string rozdzielony spacjami
    const QString args_string = args.join(' ');
    sei.lpParameters = args_string.isEmpty() ? nullptr : reinterpret_cast<LPCWSTR>(args_string.utf16());
    sei.hwnd = nullptr;
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_DEFAULT; // Można dodać SEE_MASK_NOCLOSEPROCESS jeśli potrzebujesz uchwytu

    qDebug() << "Attempting to relaunch" << app_path << "with args:" << args_string << "as admin...";

    if (ShellExecuteExW(&sei)) {
        qDebug() << "Relaunch successful (UAC prompt should appear).";
        return true;
    }
    const DWORD error = GetLastError();
    qWarning() << "ShellExecuteExW failed with error code:" << error;
    // ERROR_CANCELLED (1223) oznacza, że użytkownik anulował UAC
    if (error == ERROR_CANCELLED) {
        qDebug() << "User cancelled the UAC prompt.";
    } else {
        qWarning() << "Failed to request elevation.";
    }
    return false;
}


LRESULT CALLBACK SystemOverrideManager::LowLevelKeyboardProc(const int n_code, const WPARAM w_param, const LPARAM l_param)
{
    if (n_code == HC_ACTION)
    {
        // Sprawdź tylko zdarzenia wciśnięcia klawisza
        if (w_param == WM_KEYDOWN || w_param == WM_SYSKEYDOWN)
        {
            const auto pkhs = reinterpret_cast<KBDLLHOOKSTRUCT *>(l_param);
            const DWORD vkCode = pkhs->vkCode;

            // Sprawdź, czy klawisz jest na liście dozwolonych
            if (!kAllowedKeys.contains(vkCode))
            {
                // Klawisz NIE jest dozwolony - zablokuj go
                // qDebug() << "Blocking key:" << vkCode; // Opcjonalny debug
                return 1; // Zwrócenie wartości > 0 blokuje dalsze przetwarzanie
            }
            // Jeśli klawisz jest dozwolony, przejdź dalej
            // qDebug() << "Allowing key:" << vkCode; // Opcjonalny debug
        }
    }

    // Przekaż zdarzenie do następnego haka w łańcuchu
    // Ważne: Przekazuj m_keyboardHook, a nie twórz nowego uchwytu!
    return CallNextHookEx(keyboard_hook_, n_code, w_param, l_param);
}

bool SystemOverrideManager::InstallKeyboardHook()
{
    if (keyboard_hook_ != nullptr) {
        qWarning() << "Keyboard hook already installed.";
        return true; // Już zainstalowany
    }

    // Pobierz uchwyt do bieżącego modułu (DLL lub EXE)
    //HINSTANCE hInstance = GetModuleHandle(NULL); // Można użyć NULL dla bieżącego procesu
    // LUB jeśli SystemOverrideManager jest w DLL:
    HINSTANCE h_instance = GetModuleHandleW(L"wavelength.dll"); // Zastąp poprawną nazwą DLL jeśli dotyczy
    if (!h_instance) {
         // Jeśli powyższe zawiedzie, spróbuj z NULL
         h_instance = GetModuleHandle(nullptr);
         if (!h_instance) {
            qCritical() << "Failed to get module handle for keyboard hook. Error:" << GetLastError();
            return false;
         }
    }


    qDebug() << "Attempting to install low-level keyboard hook...";
    keyboard_hook_ = SetWindowsHookEx(
        WH_KEYBOARD_LL,             // Typ haka: nisko-poziomowy klawiatury
        LowLevelKeyboardProc,       // Wskaźnik do procedury haka
        h_instance,                  // Uchwyt do modułu zawierającego procedurę haka
        0                           // ID wątku (0 dla globalnego haka)
    );

    if (keyboard_hook_ == nullptr) {
        qCritical() << "Failed to install keyboard hook! Error code:" << GetLastError()
                   << ". Konami code might not work, and input won't be fully blocked.";
        return false;
    }

    qDebug() << "Low-level keyboard hook installed successfully. Handle:" << keyboard_hook_;
    return true;
}

void SystemOverrideManager::UninstallKeyboardHook()
{
    if (keyboard_hook_ != nullptr) {
        qDebug() << "Uninstalling low-level keyboard hook. Handle:" << keyboard_hook_;
        if (UnhookWindowsHookEx(keyboard_hook_)) {
            qDebug() << "Keyboard hook uninstalled successfully.";
        } else {
            qWarning() << "Failed to uninstall keyboard hook! Error code:" << GetLastError();
        }
        keyboard_hook_ = nullptr; // Zawsze resetuj uchwyt
    }
}

LRESULT CALLBACK SystemOverrideManager::LowLevelMouseProc(const int n_code, const WPARAM w_param, const LPARAM l_param)
{
    if (n_code == HC_ACTION)
    {
        // Blokuj wszystkie zdarzenia myszy (kliknięcia, ruch, kółko)
        // Można by ewentualnie przepuszczać WM_MOUSEMOVE, jeśli chcemy tylko blokować kliknięcia,
        // ale pełna blokada jest bezpieczniejsza dla utrzymania fokusu.
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
                // qDebug() << "Blocking mouse event:" << wParam; // Opcjonalny debug
                return 1; // Zablokuj zdarzenie
            default:
                break; // Inne zdarzenia (np. systemowe) przepuszczamy
        }
    }

    // Przekaż zdarzenie do następnego haka w łańcuchu
    return CallNextHookEx(mouse_hook_, n_code, w_param, l_param);
}

bool SystemOverrideManager::InstallMouseHook()
{
    if (mouse_hook_ != nullptr) {
        qWarning() << "Mouse hook already installed.";
        return true;
    }

    const HINSTANCE h_instance = GetModuleHandle(nullptr);
    if (!h_instance) {
        qCritical() << "Failed to get module handle for mouse hook. Error:" << GetLastError();
        return false;
    }

    qDebug() << "Attempting to install low-level mouse hook...";
    mouse_hook_ = SetWindowsHookEx(
        WH_MOUSE_LL,            // Typ haka: nisko-poziomowy myszy
        LowLevelMouseProc,      // Wskaźnik do procedury haka
        h_instance,              // Uchwyt do modułu
        0                       // ID wątku (0 dla globalnego)
    );

    if (mouse_hook_ == nullptr) {
        qCritical() << "Failed to install mouse hook! Error code:" << GetLastError();
        return false;
    }

    qDebug() << "Low-level mouse hook installed successfully. Handle:" << mouse_hook_;
    return true;
}

void SystemOverrideManager::UninstallMouseHook()
{
    if (mouse_hook_ != nullptr) {
        qDebug() << "Uninstalling low-level mouse hook. Handle:" << mouse_hook_;
        if (UnhookWindowsHookEx(mouse_hook_)) {
            qDebug() << "Mouse hook uninstalled successfully.";
        } else {
            qWarning() << "Failed to uninstall mouse hook! Error code:" << GetLastError();
        }
        mouse_hook_ = nullptr;
    }
}

bool SystemOverrideManager::RelaunchNormally(const QStringList& arguments)
{
    const QString app_path = QApplication::applicationFilePath();
    const QStringList args = arguments; // Przekaż argumenty, jeśli są potrzebne

    qDebug() << "Relaunching normally:" << app_path << "with args:" << args.join(' ');

    // Użyj QProcess::startDetached - jest prostsze i powinno wystarczyć
    if (QProcess::startDetached(app_path, args)) {
        qDebug() << "QProcess::startDetached successful for normal relaunch.";
        return true;
    }
    qWarning() << "QProcess::startDetached failed for normal relaunch.";
    // Można dodać fallback na ShellExecuteEx z 'open', ale startDetached jest preferowane
    return false;
}
#endif