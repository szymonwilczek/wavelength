#include "shortcut_manager.h"

#include <QShortcut>
#include <QMainWindow>

#include "../wavelength_config.h"
#include "../../ui/buttons/cyberpunk_button.h"
#include "../../ui/views/settings_view.h"
#include "../../ui/views/wavelength_chat_view.h"
#include "../../ui/navigation/navbar.h"
#include <concepts>

ShortcutManager* ShortcutManager::GetInstance() {
    static ShortcutManager instance;
    return &instance;
}

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent), config_(WavelengthConfig::getInstance())
{}

void ShortcutManager::RegisterShortcuts(QWidget* parent) {
    if (!parent) {
        qWarning() << "ShortcutManager::registerShortcuts: Parent widget is null.";
        return;
    }

    if (registered_shortcuts_.contains(parent)) {
        qDebug() << "[ShortcutMgr] Clearing old shortcuts for widget:" << parent->objectName();
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
    const QKeySequence sequence = config_->getShortcut(action_id); // Pobierz sekwencję (powinna być już załadowana z pliku)

    // <<< Dodane logowanie >>>
    qDebug() << "[ShortcutMgr] createAndConnectShortcut(" << action_id << "): Using sequence" << sequence.toString();
    // <<< Koniec logowania >>>

    if (sequence.isEmpty()) {
        qDebug() << "[ShortcutMgr] No key sequence defined or loaded for action:" << action_id;
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
    qDebug() << "[ShortcutMgr] Registered" << sequence.toString() << "for" << action_id << "on" << parent->objectName();
}

void ShortcutManager::updateRegisteredShortcuts() {
    qDebug() << "[ShortcutMgr] Updating registered shortcuts...";
    // Iteruj po wszystkich zarejestrowanych widgetach
    for (auto widget_iterator = registered_shortcuts_.begin(); widget_iterator != registered_shortcuts_.end(); ++widget_iterator) {
        const QWidget* parent_widget = widget_iterator.key();
        QMap<QString, QShortcut*>& shortcuts_map = widget_iterator.value(); // Mapa ActionID -> QShortcut* dla danego widgetu
        qDebug() << "[ShortcutMgr] Updating shortcuts for widget:" << (parent_widget ? parent_widget->objectName() : "NULL");

        // Iteruj po wszystkich skrótach zarejestrowanych dla tego widgetu
        for (auto shortcut_iterator = shortcuts_map.begin(); shortcut_iterator != shortcuts_map.end(); ++shortcut_iterator) {
            const QString& action_id = shortcut_iterator.key();
            QShortcut* shortcut = shortcut_iterator.value();

            if (!shortcut) {
                qWarning() << "[ShortcutMgr] Found null shortcut pointer for action:" << action_id;
                continue;
            }

            if (QKeySequence new_sequence = config_->getShortcut(action_id); shortcut->key() != new_sequence) {
                qDebug() << "[ShortcutMgr] Updating key for" << action_id << "from" << shortcut->key().toString() << "to" << new_sequence.toString();
                shortcut->setKey(new_sequence);
            }
        }
    }
    qDebug() << "[ShortcutMgr] Finished updating registered shortcuts.";
}

void ShortcutManager::RegisterMainWindowShortcuts(QMainWindow* window, Navbar* navbar) {

    CreateAndConnectShortcut("MainWindow.CreateWavelength", window, [navbar]() {
        qDebug() << "Shortcut activated: CreateWavelength";
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
        qDebug() << "Shortcut activated: JoinWavelength";
        if (navbar) emit navbar->joinWavelengthClicked();
    });

    CreateAndConnectShortcut("MainWindow.OpenSettings", window, [navbar]() {
        qDebug() << "Shortcut activated: OpenSettings";
        if (navbar) emit navbar->settingsClicked();
    });
}

void ShortcutManager::RegisterChatViewShortcuts(WavelengthChatView* chat_view) {
    // Skróty dla widoku czatu

    CreateAndConnectShortcut("ChatView.AbortConnection", chat_view, [chat_view]() {
        qDebug() << "Shortcut activated: AbortConnection";
        // Potrzebujemy dostępu do przycisku lub metody publicznej
        // Zakładając, że przycisk abortButton jest dostępny lub mamy metodę publiczną
        if (auto* button = chat_view->findChild<QPushButton*>("abortButton")) { // Jeśli ma nazwę obiektu
            button->click();
        } else {
             // Wywołaj slot publiczny, jeśli istnieje
             QMetaObject::invokeMethod(chat_view, "abortWavelength", Qt::QueuedConnection);
        }
    });

    CreateAndConnectShortcut("ChatView.FocusInput", chat_view, [chat_view]() {
        qDebug() << "Shortcut activated: FocusInput";
        // Znajdź pole po objectName
        if (auto* input = chat_view->findChild<QLineEdit*>("chatInputField")) {
            input->setFocus(Qt::ShortcutFocusReason); // Użyj odpowiedniego powodu
            input->selectAll(); // Opcjonalnie zaznacz tekst
            // Spróbuj aktywować okno nadrzędne, aby upewnić się, że ma fokus systemowy
            if (chat_view->window()) {
                chat_view->window()->activateWindow();
            }
             qDebug() << "Focus set on chatInputField";
        } else {
            qWarning() << "Could not find QLineEdit with objectName 'chatInputField'";
        }
    });

    CreateAndConnectShortcut("ChatView.AttachFile", chat_view, [chat_view]() {
        qDebug() << "Shortcut activated: AttachFile";
        chat_view->attachFile();
    });

    CreateAndConnectShortcut("ChatView.SendMessage", chat_view, [chat_view]() {
        qDebug() << "Shortcut activated: SendMessage";
        if (auto* button = chat_view->findChild<QPushButton*>("sendButton")) {
            button->click();
        } else {
             QMetaObject::invokeMethod(chat_view, "sendMessage", Qt::QueuedConnection);
        }
    });
}

void ShortcutManager::RegisterSettingsViewShortcuts(SettingsView* settings_view) {
    // Skróty dla widoku ustawień

    CreateAndConnectShortcut("SettingsView.SwitchTab0", settings_view, [settings_view]() {
        qDebug() << "Shortcut activated: SwitchTab0";
        QMetaObject::invokeMethod(settings_view, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 0));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab1", settings_view, [settings_view]() {
        qDebug() << "Shortcut activated: SwitchTab1";
        QMetaObject::invokeMethod(settings_view, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 1));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab2", settings_view, [settings_view]() {
        qDebug() << "Shortcut activated: SwitchTab2";
        QMetaObject::invokeMethod(settings_view, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 2));
    });
    CreateAndConnectShortcut("SettingsView.SwitchTab3", settings_view, [settings_view]() {
        qDebug() << "Shortcut activated: SwitchTab3";
        QMetaObject::invokeMethod(settings_view, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 3));
    });
     CreateAndConnectShortcut("SettingsView.SwitchTab4", settings_view, [settings_view]() { // Dla nowej zakładki
        qDebug() << "Shortcut activated: SwitchTab4";
        QMetaObject::invokeMethod(settings_view, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 4));
    });

    CreateAndConnectShortcut("SettingsView.Save", settings_view, [settings_view]() {
        qDebug() << "Shortcut activated: Save Settings";
        QMetaObject::invokeMethod(settings_view, "saveSettings", Qt::QueuedConnection);
    });

    CreateAndConnectShortcut("SettingsView.Defaults", settings_view, [settings_view]() {
        qDebug() << "Shortcut activated: Restore Defaults";
        QMetaObject::invokeMethod(settings_view, "restoreDefaults", Qt::QueuedConnection);
    });

    CreateAndConnectShortcut("SettingsView.Back", settings_view, [settings_view]() {
        qDebug() << "Shortcut activated: Back";
        QMetaObject::invokeMethod(settings_view, "handleBackButton", Qt::QueuedConnection);
    });
}