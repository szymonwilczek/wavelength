#include "shortcuts_settings_widget.h"
#include "../../../../util/wavelength_config.h" // Potrzebne do WavelengthConfig

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QKeySequenceEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QMessageBox>
#include <QDebug>

ShortcutsSettingsWidget::ShortcutsSettingsWidget(QWidget *parent)
    : QWidget(parent), m_config(WavelengthConfig::getInstance()), m_formLayout(nullptr), m_restoreButton(nullptr)
{
    setupUi();
    loadSettings(); // Załaduj skróty przy tworzeniu widgetu
}

void ShortcutsSettingsWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    QLabel *titleLabel = new QLabel("Keyboard Shortcuts", this);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #00ccff; background-color: transparent; border: none;");
    mainLayout->addWidget(titleLabel);

    QLabel *infoLabel = new QLabel("Customize the keyboard shortcuts for various actions.\nChanges will take effect after restarting the application.", this);
    infoLabel->setStyleSheet("color: #cccccc; background-color: transparent; border: none; font-size: 9pt;");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    // Użyj QScrollArea, aby zmieścić wszystkie skróty
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }"); // Styl dla scroll area

    QWidget *scrollWidget = new QWidget(); // Widget wewnątrz scroll area
    scrollWidget->setStyleSheet("background-color: transparent;");
    m_formLayout = new QFormLayout(scrollWidget);
    m_formLayout->setContentsMargins(10, 10, 10, 10);
    m_formLayout->setSpacing(8);
    m_formLayout->setLabelAlignment(Qt::AlignRight);

    // Pobierz wszystkie domyślne akcje, aby utworzyć pola edycji
    QMap<QString, QKeySequence> defaultShortcuts = m_config->getDefaultShortcutsMap();
    m_shortcutEdits.clear();



    for (auto it = defaultShortcuts.constBegin(); it != defaultShortcuts.constEnd(); ++it) {
        QString actionId = it.key();
        QString description = getActionDescription(actionId);
        if (description.isEmpty()) {
            qWarning() << "No description for shortcut action:" << actionId;
            continue; // Pomiń, jeśli nie ma opisu
        }

        QLabel *descLabel = new QLabel(description + ":", scrollWidget);
        descLabel->setStyleSheet("color: #ddeeff; background-color: transparent; border: none;");

        QKeySequenceEdit *keyEdit = new QKeySequenceEdit(scrollWidget);
        keyEdit->setStyleSheet(
            "QKeySequenceEdit {"
            "   background-color: #05101A;"
            "   color: #99ccff;"
            "   border: 1px solid #005577;"
            "   padding: 4px;"
            "   font-family: Consolas;"
            "}"
            "QKeySequenceEdit:focus {"
            "   border: 1px solid #00aaff;"
            "}"
        );

        // --- ZMIANA: Pobierz AKTUALNĄ wartość skrótu (domyślną lub z ustawień) ---
        keyEdit->setKeySequence(m_config->getShortcut(actionId)); // Użyj getShortcut

        m_formLayout->addRow(descLabel, keyEdit);
        m_shortcutEdits.insert(actionId, keyEdit); // Zapisz wskaźnik do widgetu edycji
    }

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea, 1); // Rozciągnij scroll area

    // Przycisk przywracania domyślnych
    m_restoreButton = new QPushButton("Restore Default Shortcuts", this);
    m_restoreButton->setStyleSheet(
        "QPushButton { background-color: #444; border: 1px solid #666; padding: 8px; border-radius: 4px; color: #ccc; }"
        "QPushButton:hover { background-color: #555; }"
        "QPushButton:pressed { background-color: #333; }"
    );
    connect(m_restoreButton, &QPushButton::clicked, this, &ShortcutsSettingsWidget::restoreDefaultShortcuts);
    mainLayout->addWidget(m_restoreButton, 0, Qt::AlignRight);
}

QString ShortcutsSettingsWidget::getActionDescription(const QString& actionId) {
    // Zwraca czytelny opis na podstawie ID akcji
    if (actionId == "MainWindow.CreateWavelength") return "Create Wavelength";
    if (actionId == "MainWindow.JoinWavelength") return "Join Wavelength";
    if (actionId == "MainWindow.OpenSettings") return "Open Settings";
    if (actionId == "ChatView.AbortConnection") return "Abort Connection";
    if (actionId == "ChatView.FocusInput") return "Focus Message Input";
    if (actionId == "ChatView.AttachFile") return "Attach File";
    if (actionId == "ChatView.SendMessage") return "Send Message";
    if (actionId == "SettingsView.SwitchTab0") return "Switch to Wavelength Tab";
    if (actionId == "SettingsView.SwitchTab1") return "Switch to Appearance Tab";
    if (actionId == "SettingsView.SwitchTab2") return "Switch to Network Tab";
    if (actionId == "SettingsView.SwitchTab3") return "Switch to CLASSIFIED Tab";
    if (actionId == "SettingsView.SwitchTab4") return "Switch to Shortcuts Tab";
    if (actionId == "SettingsView.Save") return "Save Settings";
    if (actionId == "SettingsView.Defaults") return "Restore Defaults";
    if (actionId == "SettingsView.Back") return "Back / Close Settings";
    return QString(); // Zwróć pusty, jeśli ID nieznane
}

void ShortcutsSettingsWidget::loadSettings() {
    qDebug() << "ShortcutsSettingsWidget: Loading settings...";
    for (auto it = m_shortcutEdits.constBegin(); it != m_shortcutEdits.constEnd(); ++it) {
        QString actionId = it.key();
        QKeySequenceEdit *editWidget = it.value();
        if (editWidget) {
            editWidget->setKeySequence(m_config->getShortcut(actionId));
        }
    }
}

void ShortcutsSettingsWidget::saveSettings() {
    qDebug() << "ShortcutsSettingsWidget: Saving settings...";
    bool changed = false;
    for (auto it = m_shortcutEdits.constBegin(); it != m_shortcutEdits.constEnd(); ++it) {
        QString actionId = it.key();
        QKeySequenceEdit *editWidget = it.value();
        if (editWidget) {
            QKeySequence currentSequence = m_config->getShortcut(actionId);
            QKeySequence newSequence = editWidget->keySequence();
            if (currentSequence != newSequence) {
                m_config->setShortcut(actionId, newSequence);
                changed = true;
                qDebug() << "Shortcut changed for" << actionId << "to" << newSequence.toString();
            }
        }
    }
    if (changed) {
        // Informacja dla użytkownika - zmiany wymagają restartu
        // Zapis do pliku nastąpi globalnie w SettingsView::saveSettings -> m_config->saveSettings()
        qDebug() << "Shortcuts have been modified. Restart required for changes to take effect.";
        // Można by tu pokazać QMessageBox, ale saveSettings jest wywoływane przez główny przycisk Zapisz
    }
}

void ShortcutsSettingsWidget::restoreDefaultShortcuts() {
    if (QMessageBox::question(this, "Restore Default Shortcuts",
                             "Are you sure you want to restore all keyboard shortcuts to their default values?\nThis action cannot be undone immediately.",
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        // --- ZMIANA: Ustaw domyślne wartości w mapie konfiguracji ---
        QMap<QString, QKeySequence> defaultShortcuts = m_config->getDefaultShortcutsMap(); // Potrzebujemy metody zwracającej m_defaultShortcuts
        for(auto it = defaultShortcuts.constBegin(); it != defaultShortcuts.constEnd(); ++it) {
            m_config->setShortcut(it.key(), it.value()); // Ustaw domyślny w mapie m_shortcuts
        }
        // --- KONIEC ZMIANY ---

        // Odśwież UI, aby pokazać domyślne wartości
        loadSettings();

        QMessageBox::information(this, "Defaults Restored", "Default shortcuts have been restored.\nRemember to save settings and restart the application.");
    }
}