#include "system_override_manager.h"
#include "floating_energy_sphere_widget.h" // Include dla widgetu
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSettings>
#include <random>

#ifdef Q_OS_WIN
#include <shellapi.h>
#include "shldisp.h"
#include <winuser.h>
#include <shobjidl.h> // Dla IShellDispatch (MinimizeAll)
#endif

// --- Stałe ---
const int MOUSE_SIMULATION_INTERVAL_MS = 16; // Zwiększona częstotliwość dla płynności (~60 FPS)
const int MOUSE_TARGET_INTERVAL_MS = 1500; // Jak często zmieniać cel myszy (1.5 sekundy)
const int MOUSE_SIMULATION_DURATION_MS = 15000;
const int RESTORE_DELAY_MS = 500;
const QString OVERRIDE_WALLPAPER_RESOURCE = ":/assets/images/wavelength_override_wallpaper.png";
const QString OVERRIDE_SOUND_RESOURCE = "qrc:/assets/sounds/override_ambient.wav"; // Ścieżka do pliku dźwiękowego w zasobach


SystemOverrideManager::SystemOverrideManager(QObject *parent)
    : QObject(parent),
      m_mouseMoveTimer(new QTimer(this)),
      m_mouseTargetTimer(new QTimer(this)), // Inicjalizacja timera celu
      m_trayIcon(nullptr),
      m_floatingWidget(nullptr),
      m_mediaPlayer(nullptr), // Inicjalizacja dźwięku
      m_audioOutput(nullptr), // Inicjalizacja dźwięku
      m_originalWallpaperPath(""),
      m_tempBlackWallpaperPath(""),
      m_overrideActive(false),
      m_inputBlocked(false) // Początkowo wejście nie jest zablokowane
{
    connect(m_mouseMoveTimer, &QTimer::timeout, this, &SystemOverrideManager::simulateMouseMovement);
    connect(m_mouseTargetTimer, &QTimer::timeout, this, &SystemOverrideManager::updateMouseTarget); // Połączenie timera celu

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
    CoInitialize(NULL);
#endif
}

SystemOverrideManager::~SystemOverrideManager()
{
    // Zawsze próbuj odblokować wejście przy zamykaniu
    if (m_inputBlocked) {
        blockUserInput(false);
    }
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
    CoUninitialize();
#endif
}

bool SystemOverrideManager::blockUserInput(bool block) {
#ifdef Q_OS_WIN
    qDebug() << (block ? "Blocking" : "Unblocking") << "user input...";
    if (BlockInput(block)) {
        m_inputBlocked = block;
        qDebug() << "User input" << (block ? "blocked." : "unblocked.");
        return true;
    } else {
        m_inputBlocked = false; // Upewnij się, że flaga jest poprawna w razie błędu
        qWarning() << "BlockInput failed! Error code:" << GetLastError();
        return false;
    }
#else
    qWarning() << "Input blocking not implemented for this OS.";
    return false; // Nie zablokowano
#endif
}

void SystemOverrideManager::initiateOverrideSequence(bool isFirstTime)
{
    if (m_overrideActive) {
        qWarning() << "System Override sequence already active.";
        return;
    }

    // --- KROK 0: Zablokuj wejście ---
    if (!blockUserInput(true)) {
        qWarning() << "Failed to block user input. Aborting override sequence for safety.";
        // Można wyświetlić komunikat użytkownikowi
        // QMessageBox::critical(nullptr, "Override Error", "Failed to block user input. System Override aborted.");
        // return; // Nie kontynuuj, jeśli nie można zablokować wejścia
    }

    m_overrideActive = true;
    qDebug() << "Initiating System Override Sequence...";

    // --- Kolejne kroki ---
    qDebug() << "Changing wallpaper...";
    if (!changeWallpaper(OVERRIDE_WALLPAPER_RESOURCE)) {
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

    qDebug() << "Starting mouse simulation...";
    startMouseSimulation();

    qDebug() << "Showing floating animation widget...";
    showFloatingAnimationWidget(isFirstTime); // Pokazuje widget i uruchamia dźwięk wewnątrz

    // Timer do zatrzymania symulacji myszy
    QTimer::singleShot(MOUSE_SIMULATION_DURATION_MS, this, &SystemOverrideManager::stopMouseSimulation);

    // WAŻNE: Nie ustawiamy już timera do automatycznego przywracania.
    // Przywracanie stanu powinno nastąpić TYLKO po zamknięciu widgetu animacji.
    // Widget animacji nie może być teraz zamknięty przez użytkownika.
    // Zamknięcie zainicjuje `restoreSystemState`.
}

bool SystemOverrideManager::changeWallpaper(const QString& /*resourcePath - parametr nieużywany*/)
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
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
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
    std::wstring pathW = m_tempBlackWallpaperPath.toStdWString();
    if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)pathW.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
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
        std::wstring pathW = m_originalWallpaperPath.toStdWString();
        if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)pathW.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
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

void SystemOverrideManager::startMouseSimulation()
{
#ifdef Q_OS_WIN
    POINT p;
    GetCursorPos(&p);
    m_currentMousePos = QPointF(p.x, p.y); // Zapisz początkową pozycję jako float
    updateMouseTarget(); // Ustaw pierwszy cel
    m_mouseTargetTimer->start(MOUSE_TARGET_INTERVAL_MS); // Zacznij zmieniać cel
    m_mouseMoveTimer->start(MOUSE_SIMULATION_INTERVAL_MS); // Zacznij ruch
    qDebug() << "Mouse simulation started.";
#else
    qWarning() << "Mouse simulation not implemented for this OS.";
#endif
}

void SystemOverrideManager::stopMouseSimulation()
{
    if (m_mouseMoveTimer->isActive()) {
        m_mouseMoveTimer->stop();
        m_mouseTargetTimer->stop(); // Zatrzymaj też zmianę celu
        qDebug() << "Mouse simulation stopped.";
    }
}

void SystemOverrideManager::updateMouseTarget() {
#ifdef Q_OS_WIN
    // Ustaw nowy losowy cel na ekranie
    static std::default_random_engine generator(std::random_device{}());
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    std::uniform_int_distribution<int> distX(screenGeometry.left() + 50, screenGeometry.right() - 50);
    std::uniform_int_distribution<int> distY(screenGeometry.top() + 50, screenGeometry.bottom() - 50);

    m_targetMousePos = QPointF(distX(generator), distY(generator));
    qDebug() << "New mouse target:" << m_targetMousePos;
#endif
}


void SystemOverrideManager::simulateMouseMovement()
{
#ifdef Q_OS_WIN
    // Płynny ruch w kierunku celu (prosta interpolacja)
    QPointF direction = m_targetMousePos - m_currentMousePos;
    float distance = std::sqrt(QPointF::dotProduct(direction, direction));

    // Im bliżej celu, tym wolniej
    float speedFactor = qBound(0.01f, distance / 200.0f, 0.1f); // Dostosuj dzielnik dla prędkości

    if (distance > 5.0f) { // Jeśli nie jesteśmy bardzo blisko celu
        m_currentMousePos += direction * speedFactor;
    } else {
        // Jesteśmy blisko, można ustawić nowy cel od razu lub poczekać na timer
        // updateMouseTarget(); // Opcjonalnie: zmień cel od razu po dotarciu
    }

    // Ogranicz do ekranu (na wszelki wypadek)
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    m_currentMousePos.setX(qBound((qreal)screenGeometry.left(), m_currentMousePos.x(), (qreal)screenGeometry.right() - 1));
    m_currentMousePos.setY(qBound((qreal)screenGeometry.top(), m_currentMousePos.y(), (qreal)screenGeometry.bottom() - 1));

    // Ustaw pozycję kursora
    SetCursorPos(static_cast<int>(m_currentMousePos.x()), static_cast<int>(m_currentMousePos.y()));
#else
    // Not implemented
#endif
}

bool SystemOverrideManager::sendWindowsNotification(const QString& title, const QString& message)
{
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
    IShellDispatch *pShellDispatch = NULL;
    HRESULT hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void **)&pShellDispatch);
    if (SUCCEEDED(hr)) {
        pShellDispatch->MinimizeAll();
        pShellDispatch->Release();
        qDebug() << "MinimizeAllWindows called via COM.";
        return true;
    } else {
        qWarning() << "Failed to create IShellDispatch instance. HRESULT:" << hr;
        // Fallback na starszą metodę
        HWND hwnd = FindWindowW(L"Shell_TrayWnd", NULL);
        if (hwnd) {
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

void SystemOverrideManager::showFloatingAnimationWidget(bool isFirstTime)
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

    m_floatingWidget->setClosable(false);

    connect(m_floatingWidget, &FloatingEnergySphereWidget::widgetClosed,
            this, &SystemOverrideManager::handleFloatingWidgetClosed);

    // Zamiast konamiCodeEntered, połącz nowy sygnał zakończenia sekwencji niszczenia
    // connect(m_floatingWidget, &FloatingEnergySphereWidget::konamiCodeEntered,
    //         this, &SystemOverrideManager::restoreSystemState); // <<< STARE POŁĄCZENIE
    connect(m_floatingWidget, &FloatingEnergySphereWidget::destructionSequenceFinished,
            this, &SystemOverrideManager::restoreSystemState); // <<< NOWE POŁĄCZENIE

    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - m_floatingWidget->width()) / 2;
    int y = (screenGeometry.height() - m_floatingWidget->height()) / 2;
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
    // Sprawdź, czy override był aktywny LUB czy wejście jest zablokowane
    if (!m_overrideActive && !m_inputBlocked) {
        qDebug() << "RestoreSystemState called but override is not active and input is not blocked.";
        return;
    }

    qDebug() << "Restoring system state...";

    // --- KROK 1: Odblokuj wejście ---
    // Zrób to jako pierwszy krok, aby użytkownik odzyskał kontrolę
    if (m_inputBlocked) {
        if (!blockUserInput(false)) {
            qWarning() << "CRITICAL: Failed to unblock user input during restoration!";
            // W tej sytuacji użytkownik może potrzebować restartu!
            QMessageBox::critical(nullptr, "Input Error", "Failed to restore keyboard and mouse input! You may need to restart your computer.");
        }
    } else {
         qDebug() << "Input was not blocked, skipping unblock.";
    }

    // --- Kolejne kroki (wykonywane nawet jeśli odblokowanie się nie powiodło) ---
    stopMouseSimulation();

    // Zatrzymaj i zamknij widget animacji, jeśli nadal istnieje
    if (m_floatingWidget) {
        qDebug() << "Closing existing floating widget during restore.";
        disconnect(m_floatingWidget, &FloatingEnergySphereWidget::widgetClosed, this, &SystemOverrideManager::handleFloatingWidgetClosed);
        m_floatingWidget->setClosable(true); // Pozwól na normalne zamknięcie teraz
        m_floatingWidget->close();
        m_floatingWidget = nullptr;
    }

    // Zatrzymaj dźwięk (na wszelki wypadek)
    // if (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
    //     m_mediaPlayer->stop();
    // }

    if (!restoreWallpaper()) {
        qWarning() << "Failed to restore original wallpaper.";
    }

    // Oznacz jako nieaktywny i wyemituj sygnał (tylko jeśli był aktywny)
    if(m_overrideActive) {
        m_overrideActive = false;
        qDebug() << "System state restoration complete. Override deactivated.";
        emit overrideFinished();
    } else {
        qDebug() << "System state restoration complete (input unblocked only).";
    }
}