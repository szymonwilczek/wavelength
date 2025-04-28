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

// Forward declaration
class FloatingEnergySphereWidget;

class SystemOverrideManager final : public QObject
{
    Q_OBJECT

public:
    explicit SystemOverrideManager(QObject *parent = nullptr);
    ~SystemOverrideManager() override;

#ifdef Q_OS_WIN
    static bool isRunningAsAdmin();
    static bool relaunchAsAdmin(const QStringList& arguments = QStringList());
#endif

    signals:
        void overrideFinished(); // Sygnał informujący o zakończeniu sekwencji

    public slots:
        void initiateOverrideSequence(bool isFirstTime);
    void restoreSystemState(); // Upubliczniamy, aby można było wywołać z zewnątrz

    private slots:
    void handleFloatingWidgetClosed();

private:
    bool changeWallpaper();
    bool restoreWallpaper();
    bool sendWindowsNotification(const QString& title, const QString& message) const;

    static bool minimizeAllWindows();
    void showFloatingAnimationWidget(bool isFirstTime);

    QSystemTrayIcon* m_trayIcon;
    FloatingEnergySphereWidget* m_floatingWidget;

    // Dźwięk
    QMediaPlayer* m_mediaPlayer;
    QAudioOutput* m_audioOutput; // Wymagane w Qt6

    QString m_originalWallpaperPath;
    QString m_tempBlackWallpaperPath;
    bool m_overrideActive;

#ifdef Q_OS_WIN

    static bool relaunchNormally(const QStringList& arguments = QStringList());

    // --- Hooks ---
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    static bool installKeyboardHook();

    static void uninstallKeyboardHook();
    static HHOOK m_keyboardHook;

    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

    static bool installMouseHook();

    static void uninstallMouseHook();
    static HHOOK m_mouseHook;
    // --------------------------------------------
#endif
};

#endif // SYSTEM_OVERRIDE_MANAGER_H