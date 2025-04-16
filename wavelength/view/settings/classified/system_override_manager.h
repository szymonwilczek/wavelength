#ifndef SYSTEM_OVERRIDE_MANAGER_H
#define SYSTEM_OVERRIDE_MANAGER_H

#include <QAudioOutput>
#include <QObject>
#include <QTimer>
#include <QString>
#include <QPointF> // Do płynniejszego ruchu myszy
#include <QSystemTrayIcon>
#include <QMediaPlayer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// Forward declaration
class FloatingEnergySphereWidget;

class SystemOverrideManager : public QObject
{
    Q_OBJECT

public:
    explicit SystemOverrideManager(QObject *parent = nullptr);
    ~SystemOverrideManager();

    signals:
        void overrideFinished(); // Sygnał informujący o zakończeniu sekwencji

    public slots:
        void initiateOverrideSequence(bool isFirstTime);
    void restoreSystemState(); // Upubliczniamy, aby można było wywołać z zewnątrz

    private slots:
        void simulateMouseMovement();
    void handleFloatingWidgetClosed(); // Slot do obsługi zamknięcia widgetu animacji
    void updateMouseTarget(); // Slot do zmiany celu myszy

private:
    bool changeWallpaper(const QString& imagePath);
    bool restoreWallpaper();
    void startMouseSimulation();
    void stopMouseSimulation();
    bool sendWindowsNotification(const QString& title, const QString& message);
    bool minimizeAllWindows();
    void showFloatingAnimationWidget(bool isFirstTime);
    bool blockUserInput(bool block); // Funkcja do blokowania/odblokowywania wejścia

    QTimer* m_mouseMoveTimer;
    QTimer* m_mouseTargetTimer; // Timer do zmiany celu myszy
    QSystemTrayIcon* m_trayIcon;
    FloatingEnergySphereWidget* m_floatingWidget;

    // Dźwięk
    QMediaPlayer* m_mediaPlayer;
    QAudioOutput* m_audioOutput; // Wymagane w Qt6

    QString m_originalWallpaperPath;
    bool m_overrideActive;
    bool m_inputBlocked; // Flaga śledząca stan blokady wejścia
    QPointF m_currentMousePos; // Aktualna pozycja (float dla płynności)
    QPointF m_targetMousePos; // Cel, do którego mysz ma się poruszać
};

#endif // SYSTEM_OVERRIDE_MANAGER_H