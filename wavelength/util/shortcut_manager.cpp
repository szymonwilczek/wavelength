#include "shortcut_manager.h"

#include <QWidget>
#include <QShortcut>
#include <QMainWindow>
#include <QDebug>
#include <QPushButton> // Dla click()
#include <QLineEdit>   // Dla setFocus()

#include "wavelength_config.h"
#include "../ui/button/cyberpunk_button.h"
#include "../view/settings_view.h"
#include "../view/wavelength_chat_view.h"
#include "../ui/navigation/navbar.h"

ShortcutManager* ShortcutManager::getInstance() {
    static ShortcutManager instance;
    return &instance;
}

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent), m_config(WavelengthConfig::getInstance())
{}

void ShortcutManager::registerShortcuts(QWidget* parent) {
    if (!parent) {
        qWarning() << "ShortcutManager::registerShortcuts: Parent widget is null.";
        return;
    }

    // Wyczyść stare skróty dla tego rodzica, jeśli istnieją
    if (m_registeredShortcuts.contains(parent)) {
        qDeleteAll(m_registeredShortcuts.value(parent));
        m_registeredShortcuts.remove(parent);
    }

    // Zidentyfikuj typ widgetu i zarejestruj odpowiednie skróty
    if (auto* mainWindow = qobject_cast<QMainWindow*>(parent)) {
        // Znajdź Navbar w głównym oknie
        Navbar* navbar = mainWindow->findChild<Navbar*>();
        if (navbar) {
            registerMainWindowShortcuts(mainWindow, navbar);
        } else {
            qWarning() << "ShortcutManager: Could not find Navbar in QMainWindow.";
        }
    } else if (auto* chatView = qobject_cast<WavelengthChatView*>(parent)) {
        registerChatViewShortcuts(chatView);
    } else if (auto* settingsView = qobject_cast<SettingsView*>(parent)) {
        registerSettingsViewShortcuts(settingsView);
    } else {
        qWarning() << "ShortcutManager::registerShortcuts: Unsupported parent widget type:" << parent->metaObject()->className();
    }
}

template<typename Func>
void ShortcutManager::createAndConnectShortcut(const QString& actionId, QWidget* parent, Func lambda) {
    QKeySequence sequence = m_config->getShortcut(actionId);
    if (sequence.isEmpty()) {
        qDebug() << "ShortcutManager: No key sequence defined or loaded for action:" << actionId;
        return;
    }

    QShortcut* shortcut = new QShortcut(sequence, parent);
    // Ustawienie kontekstu, aby skrót działał tylko gdy widget (lub jego dziecko) ma fokus
    shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut, &QShortcut::activated, parent, lambda);

    // Dodaj do mapy zarejestrowanych skrótów
    m_registeredShortcuts[parent].append(shortcut);
    qDebug() << "ShortcutManager: Registered" << sequence.toString() << "for" << actionId << "on" << parent->objectName();
}

void ShortcutManager::registerMainWindowShortcuts(QMainWindow* window, Navbar* navbar) {

    createAndConnectShortcut("MainWindow.CreateWavelength", window, [navbar]() {
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

    createAndConnectShortcut("MainWindow.JoinWavelength", window, [navbar]() {
        qDebug() << "Shortcut activated: JoinWavelength";
        if (navbar) emit navbar->joinWavelengthClicked();
    });

    createAndConnectShortcut("MainWindow.OpenSettings", window, [navbar]() {
        qDebug() << "Shortcut activated: OpenSettings";
        if (navbar) emit navbar->settingsClicked();
    });
}

void ShortcutManager::registerChatViewShortcuts(WavelengthChatView* chatView) {
    // Skróty dla widoku czatu

    createAndConnectShortcut("ChatView.AbortConnection", chatView, [chatView]() {
        qDebug() << "Shortcut activated: AbortConnection";
        // Potrzebujemy dostępu do przycisku lub metody publicznej
        // Zakładając, że przycisk abortButton jest dostępny lub mamy metodę publiczną
        if (auto* button = chatView->findChild<QPushButton*>("abortButton")) { // Jeśli ma nazwę obiektu
            button->click();
        } else {
             // Wywołaj slot publiczny, jeśli istnieje
             QMetaObject::invokeMethod(chatView, "abortWavelength", Qt::QueuedConnection);
        }
    });

    createAndConnectShortcut("ChatView.FocusInput", chatView, [chatView]() {
        qDebug() << "Shortcut activated: FocusInput";
        if (auto* input = chatView->findChild<QLineEdit*>("inputField")) { // Jeśli ma nazwę obiektu
            input->setFocus();
            input->selectAll(); // Opcjonalnie zaznacz tekst
        }
        // Jeśli nie ma nazwy, potrzebujemy dostępu do wskaźnika inputField
    });

    createAndConnectShortcut("ChatView.AttachFile", chatView, [chatView]() {
        qDebug() << "Shortcut activated: AttachFile";
        if (auto* button = chatView->findChild<QPushButton*>("attachButton")) {
            button->click();
        } else {
            QMetaObject::invokeMethod(chatView, "attachFile", Qt::QueuedConnection);
        }
    });

    createAndConnectShortcut("ChatView.SendMessage", chatView, [chatView]() {
        qDebug() << "Shortcut activated: SendMessage";
        if (auto* button = chatView->findChild<QPushButton*>("sendButton")) {
            button->click();
        } else {
             QMetaObject::invokeMethod(chatView, "sendMessage", Qt::QueuedConnection);
        }
    });
}

void ShortcutManager::registerSettingsViewShortcuts(SettingsView* settingsView) {
    // Skróty dla widoku ustawień

    createAndConnectShortcut("SettingsView.SwitchTab0", settingsView, [settingsView]() {
        qDebug() << "Shortcut activated: SwitchTab0";
        QMetaObject::invokeMethod(settingsView, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 0));
    });
    createAndConnectShortcut("SettingsView.SwitchTab1", settingsView, [settingsView]() {
        qDebug() << "Shortcut activated: SwitchTab1";
        QMetaObject::invokeMethod(settingsView, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 1));
    });
    createAndConnectShortcut("SettingsView.SwitchTab2", settingsView, [settingsView]() {
        qDebug() << "Shortcut activated: SwitchTab2";
        QMetaObject::invokeMethod(settingsView, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 2));
    });
    createAndConnectShortcut("SettingsView.SwitchTab3", settingsView, [settingsView]() {
        qDebug() << "Shortcut activated: SwitchTab3";
        QMetaObject::invokeMethod(settingsView, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 3));
    });
     createAndConnectShortcut("SettingsView.SwitchTab4", settingsView, [settingsView]() { // Dla nowej zakładki
        qDebug() << "Shortcut activated: SwitchTab4";
        QMetaObject::invokeMethod(settingsView, "switchToTab", Qt::QueuedConnection, Q_ARG(int, 4));
    });

    createAndConnectShortcut("SettingsView.Save", settingsView, [settingsView]() {
        qDebug() << "Shortcut activated: Save Settings";
        QMetaObject::invokeMethod(settingsView, "saveSettings", Qt::QueuedConnection);
    });

    createAndConnectShortcut("SettingsView.Defaults", settingsView, [settingsView]() {
        qDebug() << "Shortcut activated: Restore Defaults";
        QMetaObject::invokeMethod(settingsView, "restoreDefaults", Qt::QueuedConnection);
    });

    createAndConnectShortcut("SettingsView.Back", settingsView, [settingsView]() {
        qDebug() << "Shortcut activated: Back";
        QMetaObject::invokeMethod(settingsView, "handleBackButton", Qt::QueuedConnection);
    });
}