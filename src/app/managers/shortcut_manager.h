#ifndef SHORTCUT_MANAGER_H
#define SHORTCUT_MANAGER_H

#include <QObject>
#include <QKeySequence>
#include <QMap>

class QWidget;
class QShortcut;
class WavelengthConfig;
class QMainWindow;
class WavelengthChatView;
class SettingsView;
class Navbar; // Potrzebne do dostępu do przycisków

class ShortcutManager : public QObject {
    Q_OBJECT

public:
    static ShortcutManager* getInstance();

    // Rejestruje skróty dla danego widgetu (kontekstu)
    void registerShortcuts(QWidget* parent);

    public slots:
        void updateRegisteredShortcuts();

private:
    explicit ShortcutManager(QObject *parent = nullptr);
    ~ShortcutManager() override = default;
    ShortcutManager(const ShortcutManager&) = delete;
    ShortcutManager& operator=(const ShortcutManager&) = delete;

    WavelengthConfig* m_config;

    // Mapowanie widgetu na listę jego aktywnych skrótów (do ewentualnej aktualizacji)
    QMap<QWidget*, QMap<QString, QShortcut*>> m_registeredShortcuts;

    // Funkcje pomocnicze do rejestracji dla konkretnych widoków
    void registerMainWindowShortcuts(QMainWindow* window, Navbar* navbar);
    void registerChatViewShortcuts(WavelengthChatView* chatView);
    void registerSettingsViewShortcuts(SettingsView* settingsView);

    // Funkcja pomocnicza do tworzenia i łączenia skrótu
    template<typename Func>
    void createAndConnectShortcut(const QString& actionId, QWidget* parent, Func lambda);
};

#endif // SHORTCUT_MANAGER_H