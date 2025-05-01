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

class SystemOverrideManager final : public QObject
{
    Q_OBJECT

public:
    explicit SystemOverrideManager(QObject *parent = nullptr);
    ~SystemOverrideManager() override;

#ifdef Q_OS_WIN
    static bool IsRunningAsAdmin();
    static bool RelaunchAsAdmin(const QStringList& arguments = QStringList());
#endif

    signals:
        void overrideFinished();

    public slots:
        void InitiateOverrideSequence(bool is_first_time);
        void RestoreSystemState();

    private slots:
        void HandleFloatingWidgetClosed();

private:
    bool ChangeWallpaper();
    bool RestoreWallpaper();
    bool SendWindowsNotification(const QString& title, const QString& message) const;

    static bool MinimizeAllWindows();
    void ShowFloatingAnimationWidget(bool is_first_time);

    QSystemTrayIcon* tray_icon_;
    FloatingEnergySphereWidget* floating_widget_;

    QMediaPlayer* media_player_;
    QAudioOutput* audio_output_;

    QString original_wallpaper_path_;
    QString temp_black_wallpaper_path_;
    bool override_active_;

#ifdef Q_OS_WIN

    static bool RelaunchNormally(const QStringList& arguments = QStringList());

    // --- Hooks ---
    static LRESULT CALLBACK LowLevelKeyboardProc(int n_code, WPARAM w_param, LPARAM l_param);

    static bool InstallKeyboardHook();

    static void UninstallKeyboardHook();
    static HHOOK keyboard_hook_;

    static LRESULT CALLBACK LowLevelMouseProc(int n_code, WPARAM w_param, LPARAM l_param);

    static bool InstallMouseHook();

    static void UninstallMouseHook();
    static HHOOK mouse_hook_;
    // --------------------------------------------
#endif
};

#endif // SYSTEM_OVERRIDE_MANAGER_H