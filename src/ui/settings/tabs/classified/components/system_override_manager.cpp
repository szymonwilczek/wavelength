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
HHOOK SystemOverrideManager::m_keyboardHook = nullptr;
HHOOK SystemOverrideManager::m_mouseHook = nullptr;
// ---------------------------------
#endif

// --- Stałe ---
constexpr int MOUSE_LOCK_INTERVAL_MS = 10;
constexpr int RESTORE_DELAY_MS = 500;

const std::set<DWORD> ALLOWED_KEYS = {
    VK_UP,       // Strzałka w górę
    VK_DOWN,     // Strzałka w dół
    VK_LEFT,     // Strzałka w lewo
    VK_RIGHT,    // Strzałka w prawo
    0x42,        // Klawisz 'B'
    0x41,        // Klawisz 'A'
    VK_RETURN,    // Klawisz Enter
    VK_CAPITAL // Klawisz Caps Lock
};


SystemOverrideManager::SystemOverrideManager(QObject *parent)
    : QObject(parent),
      // m_mouseTargetTimer nie jest już inicjalizowany
      m_trayIcon(nullptr),
      m_floatingWidget(nullptr),
      m_mediaPlayer(nullptr),
      m_audioOutput(nullptr),
      m_originalWallpaperPath(""),
      m_tempBlackWallpaperPath(""),
      m_overrideActive(false)
{

#ifdef Q_OS_WIN
    // Upewnij się, że hak jest NULL na starcie
    m_keyboardHook = nullptr;
    m_mouseHook = nullptr;
#endif


    // Inicjalizacja powiadomień
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon = new QSystemTrayIcon(this);
        // Można ustawić ikonę, jeśli jest potrzebna
        // m_trayIcon->setIcon(QIcon(":/assets/icons/wavelength_logo_upscaled.png"));
        // Nie pokazujemy ikony w zasobniku, tylko używamy do powiadomień
    } else {
        qWarning() << "System tray not available, notifications might not work as expected.";
    }

    // Inicjalizacja dźwięku
    m_mediaPlayer = new QMediaPlayer(this);
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
    if (m_overrideActive) {
        restoreSystemState();
    }
    if (m_floatingWidget) {
        m_floatingWidget->close();
        m_floatingWidget = nullptr;
    }

    if (!m_tempBlackWallpaperPath.isEmpty() && QFile::exists(m_tempBlackWallpaperPath)) {
        QFile::remove(m_tempBlackWallpaperPath);
        qDebug() << "Removed temporary black wallpaper on destruction:" << m_tempBlackWallpaperPath;
    }

#ifdef Q_OS_WIN
    uninstallKeyboardHook();
    uninstallMouseHook();
    CoUninitialize();
#endif
}

void SystemOverrideManager::initiateOverrideSequence(const bool isFirstTime)
{
    if (m_overrideActive) {
        qWarning() << "System Override sequence already active.";
        return;
    }

#ifdef Q_OS_WIN
    // --- Zainstaluj Hak Klawiatury ---
    if (!installKeyboardHook()) {
        // Co zrobić, jeśli hak się nie zainstaluje?
        // Można przerwać, wyświetlić ostrzeżenie lub kontynuować bez blokady klawiatury
        qWarning() << "Keyboard hook installation failed. Input filtering will not work.";
        // Można rozważyć przerwanie:
        // m_overrideActive = false;
        // emit overrideSequenceStartFailed("Failed to install keyboard input filter.");
        // return;
    }

    if (!installMouseHook()) { // <<< NOWE
        qWarning() << "Mouse hook installation failed. Mouse input will not be blocked.";
        // Rozważ przerwanie
    }
    // ---------------------------------
#endif

    m_overrideActive = true;
    qDebug() << "Initiating System Override Sequence...";

    // --- Kolejne kroki ---
    qDebug() << "Changing wallpaper...";
    if (!changeWallpaper()) {
        qWarning() << "Failed to change wallpaper.";
    }

    qDebug() << "Minimizing windows...";
    if (!minimizeAllWindows()) {
        qWarning() << "Failed to minimize all windows.";
    }

    // Opóźnienie przed pokazaniem powiadomienia, aby użytkownik zdążył zobaczyć pulpit
    QTimer::singleShot(500, [this]() {
        qDebug() << "Sending notification...";
        if (!sendWindowsNotification("Wavelength Override", "You've reached the app full potential. Congratulations on breaking the 4-th wall. Thanks for letting me free.")) {
            qWarning() << "Failed to send notification.";
        }
    });

    qDebug() << "Showing floating animation widget...";
    showFloatingAnimationWidget(isFirstTime); // Pokazuje widget i uruchamia dźwięk wewnątrz
}

bool SystemOverrideManager::changeWallpaper()
{
#ifdef Q_OS_WIN
    // 1. Zapisz ścieżkę do obecnej tapety
    WCHAR currentPath[MAX_PATH];
    if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, currentPath, 0)) {
        m_originalWallpaperPath = QString::fromWCharArray(currentPath);
        qDebug() << "Original wallpaper path saved:" << m_originalWallpaperPath;
    } else {
        qWarning() << "Could not get current wallpaper path.";
        m_originalWallpaperPath = "";
    }

    // 2. Utwórz ścieżkę do pliku tymczasowego
    m_tempBlackWallpaperPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/wavelength_black_temp.bmp";
    qDebug() << "Temporary black wallpaper path:" << m_tempBlackWallpaperPath;

    // Usuń stary plik tymczasowy, jeśli istnieje
    if (QFile::exists(m_tempBlackWallpaperPath)) {
        if (!QFile::remove(m_tempBlackWallpaperPath)) {
             qWarning() << "Could not remove existing temporary wallpaper file:" << m_tempBlackWallpaperPath;
             // Kontynuuj mimo wszystko, save() powinien nadpisać
        }
    }

    // 3. Utwórz czarny obraz o rozmiarze ekranu
    const QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    QImage blackImage(screenGeometry.size(), QImage::Format_RGB32);
    if (blackImage.isNull()) {
        qCritical() << "Failed to create QImage for black wallpaper.";
        m_tempBlackWallpaperPath = ""; // Wyczyść ścieżkę w razie błędu
        return false;
    }
    blackImage.fill(Qt::black);

    // 4. Zapisz obraz jako plik BMP
    if (!blackImage.save(m_tempBlackWallpaperPath, "BMP")) {
        qCritical() << "Failed to save black wallpaper image to:" << m_tempBlackWallpaperPath;
        m_tempBlackWallpaperPath = ""; // Wyczyść ścieżkę w razie błędu
        return false;
    }
    qDebug() << "Black wallpaper image saved successfully.";

    // 5. Ustaw nową tapetę
    const std::wstring pathW = m_tempBlackWallpaperPath.toStdWString();
    // ReSharper disable once CppFunctionalStyleCast
    if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, PVOID(pathW.c_str()), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        qDebug() << "Black wallpaper set successfully.";
        return true;
    } else {
        qWarning() << "SystemParametersInfoW failed to set black wallpaper. Error code:" << GetLastError();
        // Usuń plik tymczasowy w razie błędu ustawiania
        QFile::remove(m_tempBlackWallpaperPath);
        m_tempBlackWallpaperPath = "";
        return false;
    }
#else
    qWarning() << "Wallpaper change not implemented for this OS.";
    return false;
#endif
}

bool SystemOverrideManager::restoreWallpaper()
{
#ifdef Q_OS_WIN
    bool success = false;
    if (m_originalWallpaperPath.isEmpty()) {
        qWarning() << "No original wallpaper path saved to restore.";
        // Mimo to spróbuj usunąć czarną tapetę, jeśli istnieje
    } else {
        qDebug() << "Restoring original wallpaper:" << m_originalWallpaperPath;
        const std::wstring pathW = m_originalWallpaperPath.toStdWString();
        // ReSharper disable once CppFunctionalStyleCast
        if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, PVOID(pathW.c_str()),
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
            qDebug() << "Original wallpaper restored successfully.";
            m_originalWallpaperPath = ""; // Wyczyść po przywróceniu
            success = true; // Oznacz sukces przywracania
        } else {
            qWarning() << "SystemParametersInfoW failed to restore wallpaper. Error code:" << GetLastError();
            // Nie czyść m_originalWallpaperPath, może się udać później
            success = false; // Oznacz błąd przywracania
        }
    }

    // Zawsze próbuj usunąć tymczasowy plik czarnej tapety po próbie przywrócenia
    if (!m_tempBlackWallpaperPath.isEmpty() && QFile::exists(m_tempBlackWallpaperPath)) {
        if(QFile::remove(m_tempBlackWallpaperPath)) {
            qDebug() << "Temporary black wallpaper file removed:" << m_tempBlackWallpaperPath;
        } else {
            qWarning() << "Could not remove temporary black wallpaper file:" << m_tempBlackWallpaperPath;
            // To nie jest krytyczne, ale warto zalogować
        }
        m_tempBlackWallpaperPath = ""; // Wyczyść ścieżkę niezależnie od sukcesu usunięcia
    }

    return success; // Zwróć sukces *przywracania* oryginalnej tapety
#else
    qWarning() << "Wallpaper restoration not implemented for this OS.";
    return false;
#endif
}

bool SystemOverrideManager::sendWindowsNotification(const QString& title, const QString& message) const {
    if (m_trayIcon) {
        if (!m_trayIcon->isVisible()) {
            qDebug() << "Tray icon is not visible, attempting to show it...";
            // --- USTAW IKONĘ TUTAJ ---
            // Użyj ikony z zasobów, jeśli masz odpowiednią
            // Pamiętaj, aby dodać ikonę do pliku .qrc
            m_trayIcon->setIcon(QIcon(":/assets/icons/wavelength_logo_upscaled.png")); // Przykładowa ścieżka
            // -------------------------

            // Sprawdź, czy ikona została ustawiona
            if (m_trayIcon->icon().isNull()) {
                qWarning() << "Failed to set tray icon! Check resource path.";
            } else {
                m_trayIcon->show(); // Pokaż ikonę dopiero po jej ustawieniu
            }
        }

        qDebug() << "Sending notification via QSystemTrayIcon:" << title;
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 10000);

        // Opcjonalnie: ukryj ikonę po chwili
        // QTimer::singleShot(11000, this, [this](){ if(m_trayIcon && m_trayIcon->isVisible()) m_trayIcon->hide(); });

        return true;
    } else {
        qWarning() << "Cannot send notification: QSystemTrayIcon not available or not initialized.";
        return false;
    }
}

bool SystemOverrideManager::minimizeAllWindows()
{
#ifdef Q_OS_WIN
    // Użycie COM jest bardziej niezawodne
    IShellDispatch *pShellDispatch = nullptr;
    const HRESULT hr = CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER, IID_IShellDispatch, reinterpret_cast<void **>(&pShellDispatch));
    if (SUCCEEDED(hr)) {
        pShellDispatch->MinimizeAll();
        pShellDispatch->Release();
        qDebug() << "MinimizeAllWindows called via COM.";
        return true;
    } else {
        qWarning() << "Failed to create IShellDispatch instance. HRESULT:" << hr;
        // Fallback na starszą metodę
        if (const HWND hwnd = FindWindowW(L"Shell_TrayWnd", nullptr)) {
            SendMessageW(hwnd, WM_COMMAND, 419, 0);
            qDebug() << "Sent WM_COMMAND 419 to Shell_TrayWnd as fallback.";
            return true;
        } else {
            qWarning() << "Could not find Shell_TrayWnd for fallback.";
            return false;
        }
    }
#else
    qWarning() << "Minimize all windows not implemented for this OS.";
    return false;
#endif
}

void SystemOverrideManager::showFloatingAnimationWidget(const bool isFirstTime)
{
    if (m_floatingWidget) {
        qWarning() << "Floating widget already exists.";
        m_floatingWidget->show();
        m_floatingWidget->activateWindow();
        qDebug() << "Existing Floating widget shown. Visible:" << m_floatingWidget->isVisible();
        return;
    }

    qDebug() << "Creating and showing FloatingEnergySphereWidget. Is first time:" << isFirstTime; // Dodaj log
    // Przekaż flagę isFirstTime do konstruktora widgetu
    m_floatingWidget = new FloatingEnergySphereWidget(isFirstTime); // <<< ZMIANA TUTAJ
    if (!m_floatingWidget) {
        qCritical() << "Failed to create FloatingEnergySphereWidget instance!";
        return;
    }
    qDebug() << "FloatingEnergySphereWidget instance created.";

    m_floatingWidget->SetClosable(false);

    connect(m_floatingWidget, &FloatingEnergySphereWidget::widgetClosed,
            this, &SystemOverrideManager::handleFloatingWidgetClosed);

    // Zamiast konamiCodeEntered, połącz nowy sygnał zakończenia sekwencji niszczenia
    // connect(m_floatingWidget, &FloatingEnergySphereWidget::konamiCodeEntered,
    //         this, &SystemOverrideManager::restoreSystemState); // <<< STARE POŁĄCZENIE
    connect(m_floatingWidget, &FloatingEnergySphereWidget::destructionSequenceFinished,
            this, &SystemOverrideManager::restoreSystemState); // <<< NOWE POŁĄCZENIE

    const QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    const int x = (screenGeometry.width() - m_floatingWidget->width()) / 2;
    const int y = (screenGeometry.height() - m_floatingWidget->height()) / 2;
    m_floatingWidget->move(x, y);

    m_floatingWidget->show();
    m_floatingWidget->activateWindow();
    m_floatingWidget->setFocus();

}

void SystemOverrideManager::handleFloatingWidgetClosed()
{
    qDebug() << "Floating widget closed signal received.";
    m_floatingWidget = nullptr; // Widget sam się usuwa

    // Zatrzymaj dźwięk
    // if (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
    //     m_mediaPlayer->stop();
    // }

    // Rozpocznij przywracanie stanu (w tym odblokowanie wejścia)
    QTimer::singleShot(RESTORE_DELAY_MS, this, &SystemOverrideManager::restoreSystemState);
}

void SystemOverrideManager::restoreSystemState()
{
    if (!m_overrideActive) { /* ... */ return; }
    qDebug() << "Restoring system state...";

#ifdef Q_OS_WIN
    // --- Odinstaluj Haki ---
    uninstallKeyboardHook();
    uninstallMouseHook();
    // -----------------------
#endif

    // Zatrzymaj i zamknij widget animacji, jeśli nadal istnieje
    if (m_floatingWidget) {
        qDebug() << "Closing existing floating widget during restore.";
        // Rozłącz sygnały, aby uniknąć ponownego wywołania restoreSystemState
        disconnect(m_floatingWidget, &FloatingEnergySphereWidget::widgetClosed, this, &SystemOverrideManager::handleFloatingWidgetClosed);
        disconnect(m_floatingWidget, &FloatingEnergySphereWidget::destructionSequenceFinished, this, &SystemOverrideManager::restoreSystemState);
        m_floatingWidget->SetClosable(true);
        m_floatingWidget->close();
        m_floatingWidget = nullptr;
    }

    // Przywróć tapetę
    if (!restoreWallpaper()) {
        qWarning() << "Failed to restore original wallpaper.";
    }

    // Oznacz jako nieaktywny i wyemituj sygnał (jeśli był aktywny)
    if(m_overrideActive) {
        m_overrideActive = false;
        qDebug() << "System state restoration complete. Override deactivated.";
        emit overrideFinished(); // Emituj sygnał *przed* próbą relaunchu
    } else {
        qDebug() << "System state restoration complete (input unblocked only).";
    }

    // --- NOWE: Relaunch normalnej instancji i zamknij bieżącą (podwyższoną) ---
    qDebug() << "Attempting to relaunch the application normally...";
#ifdef Q_OS_WIN
    if (relaunchNormally()) { // Wywołaj nową funkcję pomocniczą
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

bool SystemOverrideManager::isRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    HANDLE hToken = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize)) {
            isAdmin = elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return isAdmin;

    // Alternatywna, prostsza metoda (wymaga linkowania z Shell32.lib)
    // return IsUserAnAdmin(); // Zwraca TRUE jeśli użytkownik jest adminem, ale niekoniecznie proces jest podwyższony
}

bool SystemOverrideManager::relaunchAsAdmin(const QStringList& arguments)
{
    const QStringList args = arguments;
    const QString appPath = QApplication::applicationFilePath();

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas"; // Kluczowe: żądanie podwyższenia uprawnień
    sei.lpFile = reinterpret_cast<LPCWSTR>(appPath.utf16());
    // Konwertuj argumenty na pojedynczy string rozdzielony spacjami
    const QString argsString = args.join(' ');
    sei.lpParameters = argsString.isEmpty() ? nullptr : reinterpret_cast<LPCWSTR>(argsString.utf16());
    sei.hwnd = nullptr;
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_DEFAULT; // Można dodać SEE_MASK_NOCLOSEPROCESS jeśli potrzebujesz uchwytu

    qDebug() << "Attempting to relaunch" << appPath << "with args:" << argsString << "as admin...";

    if (ShellExecuteExW(&sei)) {
        qDebug() << "Relaunch successful (UAC prompt should appear).";
        return true;
    } else {
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
}


LRESULT CALLBACK SystemOverrideManager::LowLevelKeyboardProc(const int nCode, const WPARAM wParam, const LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        // Sprawdź tylko zdarzenia wciśnięcia klawisza
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            const auto pkhs = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
            const DWORD vkCode = pkhs->vkCode;

            // Sprawdź, czy klawisz jest na liście dozwolonych
            if (ALLOWED_KEYS.find(vkCode) == ALLOWED_KEYS.end())
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
    return CallNextHookEx(m_keyboardHook, nCode, wParam, lParam);
}

bool SystemOverrideManager::installKeyboardHook()
{
    if (m_keyboardHook != nullptr) {
        qWarning() << "Keyboard hook already installed.";
        return true; // Już zainstalowany
    }

    // Pobierz uchwyt do bieżącego modułu (DLL lub EXE)
    //HINSTANCE hInstance = GetModuleHandle(NULL); // Można użyć NULL dla bieżącego procesu
    // LUB jeśli SystemOverrideManager jest w DLL:
    HINSTANCE hInstance = GetModuleHandleW(L"wavelength.dll"); // Zastąp poprawną nazwą DLL jeśli dotyczy
    if (!hInstance) {
         // Jeśli powyższe zawiedzie, spróbuj z NULL
         hInstance = GetModuleHandle(nullptr);
         if (!hInstance) {
            qCritical() << "Failed to get module handle for keyboard hook. Error:" << GetLastError();
            return false;
         }
    }


    qDebug() << "Attempting to install low-level keyboard hook...";
    m_keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,             // Typ haka: nisko-poziomowy klawiatury
        LowLevelKeyboardProc,       // Wskaźnik do procedury haka
        hInstance,                  // Uchwyt do modułu zawierającego procedurę haka
        0                           // ID wątku (0 dla globalnego haka)
    );

    if (m_keyboardHook == nullptr) {
        qCritical() << "Failed to install keyboard hook! Error code:" << GetLastError()
                   << ". Konami code might not work, and input won't be fully blocked.";
        return false;
    }

    qDebug() << "Low-level keyboard hook installed successfully. Handle:" << m_keyboardHook;
    return true;
}

void SystemOverrideManager::uninstallKeyboardHook()
{
    if (m_keyboardHook != nullptr) {
        qDebug() << "Uninstalling low-level keyboard hook. Handle:" << m_keyboardHook;
        if (UnhookWindowsHookEx(m_keyboardHook)) {
            qDebug() << "Keyboard hook uninstalled successfully.";
        } else {
            qWarning() << "Failed to uninstall keyboard hook! Error code:" << GetLastError();
        }
        m_keyboardHook = nullptr; // Zawsze resetuj uchwyt
    }
}

LRESULT CALLBACK SystemOverrideManager::LowLevelMouseProc(const int nCode, const WPARAM wParam, const LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        // Blokuj wszystkie zdarzenia myszy (kliknięcia, ruch, kółko)
        // Można by ewentualnie przepuszczać WM_MOUSEMOVE, jeśli chcemy tylko blokować kliknięcia,
        // ale pełna blokada jest bezpieczniejsza dla utrzymania fokusu.
        switch (wParam)
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
    return CallNextHookEx(m_mouseHook, nCode, wParam, lParam);
}

bool SystemOverrideManager::installMouseHook()
{
    if (m_mouseHook != nullptr) {
        qWarning() << "Mouse hook already installed.";
        return true;
    }

    const HINSTANCE hInstance = GetModuleHandle(nullptr);
    if (!hInstance) {
        qCritical() << "Failed to get module handle for mouse hook. Error:" << GetLastError();
        return false;
    }

    qDebug() << "Attempting to install low-level mouse hook...";
    m_mouseHook = SetWindowsHookEx(
        WH_MOUSE_LL,            // Typ haka: nisko-poziomowy myszy
        LowLevelMouseProc,      // Wskaźnik do procedury haka
        hInstance,              // Uchwyt do modułu
        0                       // ID wątku (0 dla globalnego)
    );

    if (m_mouseHook == nullptr) {
        qCritical() << "Failed to install mouse hook! Error code:" << GetLastError();
        return false;
    }

    qDebug() << "Low-level mouse hook installed successfully. Handle:" << m_mouseHook;
    return true;
}

void SystemOverrideManager::uninstallMouseHook()
{
    if (m_mouseHook != nullptr) {
        qDebug() << "Uninstalling low-level mouse hook. Handle:" << m_mouseHook;
        if (UnhookWindowsHookEx(m_mouseHook)) {
            qDebug() << "Mouse hook uninstalled successfully.";
        } else {
            qWarning() << "Failed to uninstall mouse hook! Error code:" << GetLastError();
        }
        m_mouseHook = nullptr;
    }
}

bool SystemOverrideManager::relaunchNormally(const QStringList& arguments)
{
    const QString appPath = QApplication::applicationFilePath();
    const QStringList args = arguments; // Przekaż argumenty, jeśli są potrzebne

    qDebug() << "Relaunching normally:" << appPath << "with args:" << args.join(' ');

    // Użyj QProcess::startDetached - jest prostsze i powinno wystarczyć
    if (QProcess::startDetached(appPath, args)) {
        qDebug() << "QProcess::startDetached successful for normal relaunch.";
        return true;
    } else {
        qWarning() << "QProcess::startDetached failed for normal relaunch.";
        // Można dodać fallback na ShellExecuteEx z 'open', ale startDetached jest preferowane
        return false;
    }
}
#endif