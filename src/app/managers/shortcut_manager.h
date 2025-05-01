#ifndef SHORTCUT_MANAGER_H
#define SHORTCUT_MANAGER_H

#include <QObject>
#include <QKeySequence>
#include <QMap>
#include <concepts>

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
    static ShortcutManager* GetInstance();
    ShortcutManager(const ShortcutManager&) = delete;
    ShortcutManager& operator=(const ShortcutManager&) = delete;

    void RegisterShortcuts(QWidget* parent);

    public slots:
        void updateRegisteredShortcuts();

private:
    explicit ShortcutManager(QObject *parent = nullptr);
    ~ShortcutManager() override = default;

    WavelengthConfig* config_;

    QMap<QWidget*, QMap<QString, QShortcut*>> registered_shortcuts_;

    void RegisterMainWindowShortcuts(QMainWindow* window, Navbar* navbar);
    void RegisterChatViewShortcuts(WavelengthChatView* chat_view);
    void RegisterSettingsViewShortcuts(SettingsView* settings_view);

    template<typename Func>
    requires std::invocable<Func>
    void CreateAndConnectShortcut(const QString& action_id, QWidget* parent, Func lambda);
};

#endif // SHORTCUT_MANAGER_H