#include "shortcut_manager.h"

#include <QMainWindow>
#include <QShortcut>
#include <QDebug>
#include <QLineEdit>

#include "../wavelength_config.h"
#include "../../ui/navigation/navbar.h"
#include "../../ui/views/settings_view.h"
#include "../../ui/views/chat_view.h"
#include "../../ui/buttons/navbar_button.h"

class QPushButton;
class QLineEdit;
class CyberpunkButton;

ShortcutManager *ShortcutManager::GetInstance() {
    static ShortcutManager instance;
    return &instance;
}

ShortcutManager::ShortcutManager(QObject *parent) : QObject(parent),
                                                    config_(WavelengthConfig::GetInstance()) {
}

void ShortcutManager::RegisterShortcuts(QWidget *parent) {
    if (!parent) {
        qWarning() << "[SHORTCUT MANAGER] Parent widget is null.";
        return;
    }

    if (registered_shortcuts_.contains(parent)) {
        QMap<QString, QShortcut *> &shortcuts_map = registered_shortcuts_[parent];
        qDeleteAll(shortcuts_map.values());
        shortcuts_map.clear();
    } else {
        registered_shortcuts_.insert(parent, QMap<QString, QShortcut *>());
    }

    if (auto *main_window = qobject_cast<QMainWindow *>(parent)) {
        if (const auto navbar = main_window->findChild<Navbar *>()) {
            RegisterMainWindowShortcuts(main_window, navbar);
        } else {
            qWarning() << "[SHORTCUT MANAGER] Could not find Navbar in QMainWindow.";
        }
    } else if (auto *chat_view = qobject_cast<ChatView *>(parent)) {
        RegisterChatViewShortcuts(chat_view);
    } else if (auto *settings_view = qobject_cast<SettingsView *>(parent)) {
        RegisterSettingsViewShortcuts(settings_view);
    } else {
        qWarning() << "[SHORTCUT MANAGER] Unsupported parent widget type:" << parent->metaObject()->className();
    }
}

void ShortcutManager::updateRegisteredShortcuts() {
    for (auto widget_iterator = registered_shortcuts_.begin(); widget_iterator != registered_shortcuts_.end(); ++
         widget_iterator) {
        QMap<QString, QShortcut *> &shortcuts_map = widget_iterator.value();

        for (auto shortcut_iterator = shortcuts_map.begin(); shortcut_iterator != shortcuts_map.end(); ++
             shortcut_iterator) {
            const QString &action_id = shortcut_iterator.key();
            QShortcut *shortcut = shortcut_iterator.value();

            if (!shortcut) {
                qWarning() << "[SHORTCUT MANAGER] Found null shortcut pointer for action:" << action_id;
                continue;
            }

            if (QKeySequence new_sequence = config_->GetShortcut(action_id); shortcut->key() != new_sequence) {
                shortcut->setKey(new_sequence);
            }
        }
    }
}

void ShortcutManager::RegisterMainWindowShortcuts(QMainWindow *window, Navbar *navbar) {
    CreateAndConnectShortcut("MainWindow.CreateWavelength", window, [navbar] {
        if (navbar && navbar->findChild<CyberpunkButton *>("createWavelengthButton")) {
            navbar->findChild<CyberpunkButton *>("createWavelengthButton")->click();
        } else if (navbar) {
            emit navbar->createWavelengthClicked();
        }
    });

    CreateAndConnectShortcut("MainWindow.JoinWavelength", window, [navbar] {
        if (navbar) emit navbar->joinWavelengthClicked();
    });

    CreateAndConnectShortcut("MainWindow.OpenSettings", window, [navbar] {
        if (navbar) emit navbar->settingsClicked();
    });
}

void ShortcutManager::RegisterChatViewShortcuts(ChatView *chat_view) {
    CreateAndConnectShortcut("ChatView.AbortConnection", chat_view, [chat_view] {
        if (auto *button = chat_view->findChild<QPushButton *>("abortButton")) {
            button->click();
        } else {
            QMetaObject::invokeMethod(chat_view, "AbortWavelength", Qt::QueuedConnection);
        }
    });

    CreateAndConnectShortcut("ChatView.FocusInput", chat_view, [chat_view] {
        if (auto *input = chat_view->findChild<QLineEdit *>("chatInputField")) {
            input->setFocus(Qt::ShortcutFocusReason);
            input->selectAll();
            if (chat_view->window()) {
                chat_view->window()->activateWindow();
            }
        } else {
            qWarning() << "[SHORTCUT MANAGER] Could not find QLineEdit with objectName 'chatInputField'";
        }
    });

    CreateAndConnectShortcut("ChatView.AttachFile", chat_view, [chat_view] {
        chat_view->AttachFile();
    });

    CreateAndConnectShortcut("ChatView.SendMessage", chat_view, [chat_view] {
        if (auto *button = chat_view->findChild<QPushButton *>("sendButton")) {
            button->click();
        } else {
            QMetaObject::invokeMethod(chat_view, "SendMessage", Qt::QueuedConnection);
        }
    });
}

void ShortcutManager::RegisterSettingsViewShortcuts(SettingsView *settings_view) {
    CreateAndConnectShortcut("SettingsView.SwitchTab0", settings_view, [settings_view] {
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 0));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab1", settings_view, [settings_view] {
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 1));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab2", settings_view, [settings_view] {
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 2));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab3", settings_view, [settings_view] {
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 3));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab4", settings_view, [settings_view] {
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 4));
    });

    CreateAndConnectShortcut("SettingsView.Save", settings_view, [settings_view] {
        QMetaObject::invokeMethod(settings_view, "SaveSettings", Qt::QueuedConnection);
    });

    CreateAndConnectShortcut("SettingsView.Defaults", settings_view, [settings_view] {
        QMetaObject::invokeMethod(settings_view, "RestoreDefaults", Qt::QueuedConnection);
    });

    CreateAndConnectShortcut("SettingsView.Back", settings_view, [settings_view] {
        QMetaObject::invokeMethod(settings_view, "HandleBackButton", Qt::QueuedConnection);
    });
}

template<typename Func>
    requires std::invocable<Func>
void ShortcutManager::CreateAndConnectShortcut(const QString &action_id, QWidget *parent, Func lambda) {
    const QKeySequence sequence = config_->GetShortcut(action_id);

    if (sequence.isEmpty()) return;

    auto shortcut = new QShortcut(sequence, parent);
    shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut, &QShortcut::activated, parent, lambda);

    if (!registered_shortcuts_.contains(parent)) {
        registered_shortcuts_.insert(parent, QMap<QString, QShortcut *>());
    }
    registered_shortcuts_[parent].insert(action_id, shortcut);
}
