#include "shortcut_manager.h"

#include <QShortcut>
#include <QMainWindow>

#include "../wavelength_config.h"
#include "../../ui/buttons/navbar_button.h"
#include "../../ui/views/settings_view.h"
#include "../../ui/views/wavelength_chat_view.h"
#include "../../ui/navigation/navbar.h"
#include <concepts>

ShortcutManager* ShortcutManager::GetInstance() {
    static ShortcutManager instance;
    return &instance;
}

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent), config_(WavelengthConfig::GetInstance())
{}

void ShortcutManager::RegisterShortcuts(QWidget* parent) {
    if (!parent) {
        qWarning() << "ShortcutManager::registerShortcuts: Parent widget is null.";
        return;
    }

    if (registered_shortcuts_.contains(parent)) {
        QMap<QString, QShortcut*>& shortcuts_map = registered_shortcuts_[parent];
        qDeleteAll(shortcuts_map.values());
        shortcuts_map.clear();
    } else {
        registered_shortcuts_.insert(parent, QMap<QString, QShortcut*>());
    }

    // Zidentyfikuj typ widgetu i zarejestruj odpowiednie skróty
    if (auto* main_window = qobject_cast<QMainWindow*>(parent)) {
        if (const auto navbar = main_window->findChild<Navbar*>()) {
            RegisterMainWindowShortcuts(main_window, navbar);
        } else { qWarning() << "[ShortcutMgr] Could not find Navbar in QMainWindow."; }
    } else if (auto* chat_view = qobject_cast<WavelengthChatView*>(parent)) {
        RegisterChatViewShortcuts(chat_view);
    } else if (auto* settings_view = qobject_cast<SettingsView*>(parent)) {
        RegisterSettingsViewShortcuts(settings_view);
    } else {
        qWarning() << "[ShortcutMgr] Unsupported parent widget type:" << parent->metaObject()->className();
    }
}

template<typename Func>
requires std::invocable<Func>
void ShortcutManager::CreateAndConnectShortcut(const QString& action_id, QWidget* parent, Func lambda) {
    const QKeySequence sequence = config_->GetShortcut(action_id); // Pobierz sekwencję (powinna być już załadowana z pliku)

    if (sequence.isEmpty()) {
        return;
    }

    auto shortcut = new QShortcut(sequence, parent);
    shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut, &QShortcut::activated, parent, lambda);

    // <<< ZMIANA: Dodaj do nowej struktury mapy >>>
    // Upewnij się, że klucz 'parent' istnieje (powinien po registerShortcuts)
    if (!registered_shortcuts_.contains(parent)) {
        registered_shortcuts_.insert(parent, QMap<QString, QShortcut*>());
    }
    registered_shortcuts_[parent].insert(action_id, shortcut);
}

void ShortcutManager::updateRegisteredShortcuts() {
    // Iteruj po wszystkich zarejestrowanych widgetach
    for (auto widget_iterator = registered_shortcuts_.begin(); widget_iterator != registered_shortcuts_.end(); ++widget_iterator) {
        const QWidget* parent_widget = widget_iterator.key();
        QMap<QString, QShortcut*>& shortcuts_map = widget_iterator.value(); // Mapa ActionID -> QShortcut* dla danego widgetu

        // Iteruj po wszystkich skrótach zarejestrowanych dla tego widgetu
        for (auto shortcut_iterator = shortcuts_map.begin(); shortcut_iterator != shortcuts_map.end(); ++shortcut_iterator) {
            const QString& action_id = shortcut_iterator.key();
            QShortcut* shortcut = shortcut_iterator.value();

            if (!shortcut) {
                qWarning() << "[ShortcutMgr] Found null shortcut pointer for action:" << action_id;
                continue;
            }

            if (QKeySequence new_sequence = config_->GetShortcut(action_id); shortcut->key() != new_sequence) {
                shortcut->setKey(new_sequence);
            }
        }
    }
}

void ShortcutManager::RegisterMainWindowShortcuts(QMainWindow* window, Navbar* navbar) {

    CreateAndConnectShortcut("MainWindow.CreateWavelength", window, [navbar]() {
        // Bezpieczne sprawdzenie wskaźników przed użyciem
        if (navbar && navbar->findChild<CyberpunkButton*>("createWavelengthButton")) { // Zakładając, że przyciski mają nazwy obiektów
             navbar->findChild<CyberpunkButton*>("createWavelengthButton")->click();
        } else if (navbar) { // Jeśli nie ma nazwy, spróbuj przez wskaźnik (mniej bezpieczne jeśli się zmieni)
             // Potrzebujemy dostępu do prywatnych pól navbar lub publicznych metod/sygnałów
             // Na razie symulujemy kliknięcie przez sygnał samego navbara
             emit navbar->createWavelengthClicked(); // Zakładając, że sygnał istnieje i jest publiczny
        }
    });

    CreateAndConnectShortcut("MainWindow.JoinWavelength", window, [navbar]() {
        if (navbar) emit navbar->joinWavelengthClicked();
    });

    CreateAndConnectShortcut("MainWindow.OpenSettings", window, [navbar]() {
        if (navbar) emit navbar->settingsClicked();
    });
}

void ShortcutManager::RegisterChatViewShortcuts(WavelengthChatView* chat_view) {
    // Skróty dla widoku czatu

    CreateAndConnectShortcut("ChatView.AbortConnection", chat_view, [chat_view]() {
        // Potrzebujemy dostępu do przycisku lub metody publicznej
        // Zakładając, że przycisk abortButton jest dostępny lub mamy metodę publiczną
        if (auto* button = chat_view->findChild<QPushButton*>("abortButton")) { // Jeśli ma nazwę obiektu
            button->click();
        } else {
             // Wywołaj slot publiczny, jeśli istnieje
             QMetaObject::invokeMethod(chat_view, "AbortWavelength", Qt::QueuedConnection);
        }
    });

    CreateAndConnectShortcut("ChatView.FocusInput", chat_view, [chat_view]() {
        // Znajdź pole po objectName
        if (auto* input = chat_view->findChild<QLineEdit*>("chatInputField")) {
            input->setFocus(Qt::ShortcutFocusReason); // Użyj odpowiedniego powodu
            input->selectAll(); // Opcjonalnie zaznacz tekst
            // Spróbuj aktywować okno nadrzędne, aby upewnić się, że ma fokus systemowy
            if (chat_view->window()) {
                chat_view->window()->activateWindow();
            }
        } else {
            qWarning() << "Could not find QLineEdit with objectName 'chatInputField'";
        }
    });

    CreateAndConnectShortcut("ChatView.AttachFile", chat_view, [chat_view]() {
        chat_view->AttachFile();
    });

    CreateAndConnectShortcut("ChatView.SendMessage", chat_view, [chat_view]() {
        if (auto* button = chat_view->findChild<QPushButton*>("sendButton")) {
            button->click();
        } else {
             QMetaObject::invokeMethod(chat_view, "SendMessage", Qt::QueuedConnection);
        }
    });
}

void ShortcutManager::RegisterSettingsViewShortcuts(SettingsView* settings_view) {
    // Skróty dla widoku ustawień

    CreateAndConnectShortcut("SettingsView.SwitchTab0", settings_view, [settings_view]() {
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 0));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab1", settings_view, [settings_view]() {
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 1));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab2", settings_view, [settings_view]() {
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 2));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab3", settings_view, [settings_view]() {
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 3));
    });
     CreateAndConnectShortcut("SettingsView.SwitchTab4", settings_view, [settings_view]() { // Dla nowej zakładki
        QMetaObject::invokeMethod(settings_view, "SwitchToTab", Qt::QueuedConnection, Q_ARG(int, 4));
    });

    CreateAndConnectShortcut("SettingsView.Save", settings_view, [settings_view]() {
        QMetaObject::invokeMethod(settings_view, "SaveSettings", Qt::QueuedConnection);
    });

    CreateAndConnectShortcut("SettingsView.Defaults", settings_view, [settings_view]() {
        QMetaObject::invokeMethod(settings_view, "RestoreDefaults", Qt::QueuedConnection);
    });

    CreateAndConnectShortcut("SettingsView.Back", settings_view, [settings_view]() {
        QMetaObject::invokeMethod(settings_view, "HandleBackButton", Qt::QueuedConnection);
    });
}