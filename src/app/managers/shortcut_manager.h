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
class Navbar;

class ShortcutManager final : public QObject {
    Q_OBJECT

public:
    static ShortcutManager* getInstance();

    void registerShortcuts(QWidget* parent);

    public slots:
        void updateRegisteredShortcuts();

private:
    explicit ShortcutManager(QObject *parent = nullptr);
    ~ShortcutManager() override = default;
    ShortcutManager(const ShortcutManager&) = delete;
    ShortcutManager& operator=(const ShortcutManager&) = delete;

    WavelengthConfig* m_config;

    QMap<QWidget*, QMap<QString, QShortcut*>> m_registeredShortcuts;

    void registerMainWindowShortcuts(QMainWindow* window, Navbar* navbar);
    void registerChatViewShortcuts(WavelengthChatView* chatView);
    void registerSettingsViewShortcuts(SettingsView* settingsView);

    template<typename Func>
    void createAndConnectShortcut(const QString& actionId, QWidget* parent, Func lambda);
};

#endif // SHORTCUT_MANAGER_H