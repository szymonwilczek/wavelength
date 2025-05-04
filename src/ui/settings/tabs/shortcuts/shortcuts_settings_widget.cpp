#include "shortcuts_settings_widget.h"
#include "../../../../app/wavelength_config.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QKeySequenceEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QMessageBox>
#include <QDebug>

ShortcutsSettingsWidget::ShortcutsSettingsWidget(QWidget *parent)
    : QWidget(parent), config_(WavelengthConfig::GetInstance()), form_layout_(nullptr), restore_button_(nullptr)
{
    SetupUi();
    LoadSettings(); // Załaduj skróty przy tworzeniu widgetu
}

void ShortcutsSettingsWidget::SetupUi() {
    const auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(15, 15, 15, 15);
    main_layout->setSpacing(10);

    const auto title_label = new QLabel("Keyboard Shortcuts", this);
    title_label->setStyleSheet("font-size: 14pt; font-weight: bold; color: #00ccff; background-color: transparent; border: none;");
    main_layout->addWidget(title_label);

    const auto info_label = new QLabel("Customize the keyboard shortcuts for various actions.\nChanges will take effect after restarting the application.", this);
    info_label->setStyleSheet("color: #ffcc00; background-color: transparent; border: none; font-size: 9pt;");
    info_label->setWordWrap(true);
    main_layout->addWidget(info_label);

    // Użyj QScrollArea, aby zmieścić wszystkie skróty
    const auto scroll_area = new QScrollArea(this);
    scroll_area->setWidgetResizable(true);
    scroll_area->setStyleSheet("QScrollArea { background-color: transparent; border: none; }"); // Styl dla scroll area

    const auto scroll_widget = new QWidget(); // Widget wewnątrz scroll area
    scroll_widget->setStyleSheet("background-color: transparent;");
    form_layout_ = new QFormLayout(scroll_widget);
    form_layout_->setContentsMargins(10, 10, 10, 10);
    form_layout_->setSpacing(8);
    form_layout_->setLabelAlignment(Qt::AlignRight);

    // Pobierz wszystkie domyślne akcje, aby utworzyć pola edycji
    const QMap<QString, QKeySequence> default_shortcuts = config_->GetDefaultShortcutsMap();
    shortcut_edits_.clear();



    for (auto it = default_shortcuts.constBegin(); it != default_shortcuts.constEnd(); ++it) {
        QString action_id = it.key();
        QString description = GetActionDescription(action_id);
        if (description.isEmpty()) {
            qWarning() << "No description for shortcut action:" << action_id;
            continue; // Pomiń, jeśli nie ma opisu
        }

        const auto desc_label = new QLabel(description + ":", scroll_widget);
        desc_label->setStyleSheet("color: #ddeeff; background-color: transparent; border: none;");

        auto key_edit = new QKeySequenceEdit(scroll_widget);
        key_edit->setStyleSheet(
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
        key_edit->setKeySequence(config_->GetShortcut(action_id)); // Użyj getShortcut

        form_layout_->addRow(desc_label, key_edit);
        shortcut_edits_.insert(action_id, key_edit); // Zapisz wskaźnik do widgetu edycji
    }

    scroll_area->setWidget(scroll_widget);
    main_layout->addWidget(scroll_area, 1); // Rozciągnij scroll area

    // Przycisk przywracania domyślnych
    restore_button_ = new QPushButton("Restore Default Shortcuts", this);
    restore_button_->setStyleSheet(
        "QPushButton { background-color: #444; border: 1px solid #666; padding: 8px; border-radius: 4px; color: #ccc; }"
        "QPushButton:hover { background-color: #555; }"
        "QPushButton:pressed { background-color: #333; }"
    );
    connect(restore_button_, &QPushButton::clicked, this, &ShortcutsSettingsWidget::RestoreDefaultShortcuts);
    main_layout->addWidget(restore_button_, 0, Qt::AlignRight);
}

QString ShortcutsSettingsWidget::GetActionDescription(const QString& action_id) {
    // Zwraca czytelny opis na podstawie ID akcji
    if (action_id == "MainWindow.CreateWavelength") return "Create Wavelength";
    if (action_id == "MainWindow.JoinWavelength") return "Join Wavelength";
    if (action_id == "MainWindow.OpenSettings") return "Open Settings";
    if (action_id == "ChatView.AbortConnection") return "Abort Connection";
    if (action_id == "ChatView.FocusInput") return "Focus Message Input";
    if (action_id == "ChatView.AttachFile") return "Attach File";
    if (action_id == "ChatView.SendMessage") return "Send Message";
    if (action_id == "SettingsView.SwitchTab0") return "Switch to Wavelength Tab";
    if (action_id == "SettingsView.SwitchTab1") return "Switch to Appearance Tab";
    if (action_id == "SettingsView.SwitchTab2") return "Switch to Network Tab";
    if (action_id == "SettingsView.SwitchTab3") return "Switch to CLASSIFIED Tab";
    if (action_id == "SettingsView.SwitchTab4") return "Switch to Shortcuts Tab";
    if (action_id == "SettingsView.Save") return "Save Settings";
    if (action_id == "SettingsView.Defaults") return "Restore Defaults";
    if (action_id == "SettingsView.Back") return "Back / Close Settings";
    return QString(); // Zwróć pusty, jeśli ID nieznane
}

void ShortcutsSettingsWidget::LoadSettings() const {
    for (auto it = shortcut_edits_.constBegin(); it != shortcut_edits_.constEnd(); ++it) {
        QString action_id = it.key();
        if (QKeySequenceEdit *editWidget = it.value()) {
            editWidget->setKeySequence(config_->GetShortcut(action_id));
        }
    }
}

void ShortcutsSettingsWidget::SaveSettings() const {
    bool changed = false;
    for (auto it = shortcut_edits_.constBegin(); it != shortcut_edits_.constEnd(); ++it) {
        QString action_id = it.key();
        if (const QKeySequenceEdit *editWidget = it.value()) {
            QKeySequence current_sequence = config_->GetShortcut(action_id);
            QKeySequence new_sequence = editWidget->keySequence();
            if (current_sequence != new_sequence) {
                config_->SetShortcut(action_id, new_sequence);
                changed = true;
            }
        }
    }
}

void ShortcutsSettingsWidget::RestoreDefaultShortcuts() {
    if (QMessageBox::question(this, "Restore Default Shortcuts",
                             "Are you sure you want to restore all keyboard shortcuts to their default values?\nThis action cannot be undone immediately.",
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        // --- ZMIANA: Ustaw domyślne wartości w mapie konfiguracji ---
        const QMap<QString, QKeySequence> default_shortcuts = config_->GetDefaultShortcutsMap(); // Potrzebujemy metody zwracającej m_defaultShortcuts
        for(auto it = default_shortcuts.constBegin(); it != default_shortcuts.constEnd(); ++it) {
            config_->SetShortcut(it.key(), it.value()); // Ustaw domyślny w mapie m_shortcuts
        }
        // --- KONIEC ZMIANY ---

        // Odśwież UI, aby pokazać domyślne wartości
        LoadSettings();

        QMessageBox::information(this, "Defaults Restored", "Default shortcuts have been restored.\nRemember to save settings and restart the application.");
    }
}