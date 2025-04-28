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

    void loadSettings() const;
    void saveSettings() const; // Zapisuje zmiany dokonane w UI

private slots:
    void restoreDefaultShortcuts();

private:
    void setupUi();

    static QString getActionDescription(const QString& actionId); // Zwraca czytelny opis akcji

    WavelengthConfig *m_config;
    QFormLayout *m_formLayout;
    QPushButton *m_restoreButton;

    // Mapa przechowująca widgety edycji dla każdej akcji
    QMap<QString, QKeySequenceEdit*> m_shortcutEdits;
};

#endif // SHORTCUTS_SETTINGS_WIDGET_H