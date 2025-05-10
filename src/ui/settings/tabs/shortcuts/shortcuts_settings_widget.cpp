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

#include "../../../../app/managers/translation_manager.h"

ShortcutsSettingsWidget::ShortcutsSettingsWidget(QWidget *parent)
    : QWidget(parent), config_(WavelengthConfig::GetInstance()), form_layout_(nullptr), restore_button_(nullptr) {
    translator_ = TranslationManager::GetInstance();
    SetupUi();
    LoadSettings();
}

void ShortcutsSettingsWidget::SetupUi() {
    const auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(15, 15, 15, 15);
    main_layout->setSpacing(10);

    const auto title_label = new QLabel(
        translator_->Translate("ShortcutSettingsWidget.Title", "Keyboard Shortcuts"), this);
    title_label->setStyleSheet(
        "font-size: 14pt; font-weight: bold; color: #00ccff; background-color: transparent; border: none;");
    main_layout->addWidget(title_label);

    const auto info_label = new QLabel(
        translator_->Translate("ShortcutSettingsWidget.Info",
                               "Customize the keyboard shortcuts for various actions.\nChanges will take effect after restarting the application."),
        this);
    info_label->setStyleSheet("color: #ffcc00; background-color: transparent; border: none; font-size: 9pt;");
    info_label->setWordWrap(true);
    main_layout->addWidget(info_label);

    const auto scroll_area = new QScrollArea(this);
    scroll_area->setWidgetResizable(true);
    scroll_area->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");

    const auto scroll_widget = new QWidget();
    scroll_widget->setStyleSheet("background-color: transparent;");
    form_layout_ = new QFormLayout(scroll_widget);
    form_layout_->setContentsMargins(10, 10, 10, 10);
    form_layout_->setSpacing(8);
    form_layout_->setLabelAlignment(Qt::AlignRight);

    const QMap<QString, QKeySequence> default_shortcuts = config_->GetDefaultShortcutsMap();
    shortcut_edits_.clear();


    for (auto it = default_shortcuts.constBegin(); it != default_shortcuts.constEnd(); ++it) {
        const QString &action_id = it.key();
        QString description = GetActionDescription(action_id);
        if (description.isEmpty()) {
            qWarning() << "[SHORTCUTS TAB] No description for shortcut action:" << action_id;
            continue;
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

        key_edit->setKeySequence(config_->GetShortcut(action_id));

        form_layout_->addRow(desc_label, key_edit);
        shortcut_edits_.insert(action_id, key_edit);
    }

    scroll_area->setWidget(scroll_widget);
    main_layout->addWidget(scroll_area, 1);

    restore_button_ = new QPushButton(
        translator_->Translate("ShortcutSettingsWidget.RestoreDefaultShortcuts", "Restore Default Shortcuts"), this);
    restore_button_->setStyleSheet(
        "QPushButton { background-color: #444; border: 1px solid #666; padding: 8px; border-radius: 4px; color: #ccc; }"
        "QPushButton:hover { background-color: #555; }"
        "QPushButton:pressed { background-color: #333; }"
    );
    connect(restore_button_, &QPushButton::clicked, this, &ShortcutsSettingsWidget::RestoreDefaultShortcuts);
    main_layout->addWidget(restore_button_, 0, Qt::AlignRight);
}

QString ShortcutsSettingsWidget::GetActionDescription(const QString &action_id) {
    if (action_id == "MainWindow.CreateWavelength")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.CreateWavelength", "Create Wavelength");
    if (action_id == "MainWindow.JoinWavelength")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.JoinWavelength", "Join Wavelength");
    if (action_id == "MainWindow.OpenSettings")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.OpenSettings", "Open Settings");
    if (action_id == "ChatView.AbortConnection")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.AbortConnection", "Abort Connection");
    if (action_id == "ChatView.FocusInput")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.FocusMessageInput", "Focus Message Input");
    if (action_id == "ChatView.AttachFile")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.AttachFile", "Attach File");
    if (action_id == "ChatView.SendMessage")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.SendMessage", "Send Message");
    if (action_id == "SettingsView.SwitchTab0")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.SwitchToWavelengthTab", "Switch to Wavelength Tab");
    if (action_id == "SettingsView.SwitchTab1")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.SwitchToAppearanceTab", "Switch to Appearance Tab");
    if (action_id == "SettingsView.SwitchTab2")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.SwitchToNetworkTab", "Switch to Network Tab");
    if (action_id == "SettingsView.SwitchTab3")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.SwitchToClassifiedTab", "Switch to CLASSIFIED Tab");
    if (action_id == "SettingsView.SwitchTab4")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.SwitchToShortcutsTab", "Switch to Shortcuts Tab");
    if (action_id == "SettingsView.Save")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.SaveSettings", "Save Settings");
    if (action_id == "SettingsView.Defaults")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.RestoreDefaults", "Restore Defaults");
    if (action_id == "SettingsView.Back")
        return TranslationManager::GetInstance()->Translate(
            "ShortcutSettingsWidget.BackCloseSettings", "Back / Close Settings");
    return {};
}

void ShortcutsSettingsWidget::LoadSettings() const {
    for (auto it = shortcut_edits_.constBegin(); it != shortcut_edits_.constEnd(); ++it) {
        const QString &action_id = it.key();
        if (QKeySequenceEdit *editWidget = it.value()) {
            editWidget->setKeySequence(config_->GetShortcut(action_id));
        }
    }
}

void ShortcutsSettingsWidget::SaveSettings() const {
    for (auto it = shortcut_edits_.constBegin(); it != shortcut_edits_.constEnd(); ++it) {
        const QString &action_id = it.key();
        if (const QKeySequenceEdit *editWidget = it.value()) {
            QKeySequence current_sequence = config_->GetShortcut(action_id);
            QKeySequence new_sequence = editWidget->keySequence();
            if (current_sequence != new_sequence) {
                config_->SetShortcut(action_id, new_sequence);
            }
        }
    }
}

void ShortcutsSettingsWidget::RestoreDefaultShortcuts() {
    if (QMessageBox::question(
            this,
            translator_->Translate("ShortcutSettingsWidget.RestoreDefaultTitle", "Restore Default Shortcuts"),
            translator_->Translate("ShortcutSettingsWidget.RestoreDefaultMessage",
                                   "Are you sure you want to restore all keyboard shortcuts to their default values?\nThis action cannot be undone immediately."),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        const QMap<QString, QKeySequence> default_shortcuts = config_->GetDefaultShortcutsMap();
        for (auto it = default_shortcuts.constBegin(); it != default_shortcuts.constEnd(); ++it) {
            config_->SetShortcut(it.key(), it.value());
        }

        LoadSettings();

        QMessageBox::information(this,
                                 translator_->Translate("ShortcutSettingsWidget.RestoreDefaultSuccessTitle",
                                                        "Defaults Restored"),
                                 translator_->Translate("ShortcutSettingsWidget.RestoreDefaultSuccessMessage",
                                                        "Default shortcuts have been restored.\nRemember to save settings and restart the application."));
    }
}
