#ifndef SHORTCUTS_SETTINGS_WIDGET_H
#define SHORTCUTS_SETTINGS_WIDGET_H

#include <QWidget>
#include <QMap>

class WavelengthConfig;
class QFormLayout;
class QKeySequenceEdit;
class QLabel;
class QPushButton;

class ShortcutsSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ShortcutsSettingsWidget(QWidget *parent = nullptr);
    ~ShortcutsSettingsWidget() override = default;

    void LoadSettings() const;
    void SaveSettings() const; // Zapisuje zmiany dokonane w UI

private slots:
    void RestoreDefaultShortcuts();

private:
    void SetupUi();

    static QString GetActionDescription(const QString& action_id); // Zwraca czytelny opis akcji

    WavelengthConfig *config_;
    QFormLayout *form_layout_;
    QPushButton *restore_button_;

    QMap<QString, QKeySequenceEdit*> shortcut_edits_;
};

#endif // SHORTCUTS_SETTINGS_WIDGET_H