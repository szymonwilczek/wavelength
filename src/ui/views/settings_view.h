#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H

#include <QStackedWidget>
#include <QDateTime>

#include "../../ui/buttons/cyber_button.h"
#include "../../ui/buttons/tab_button.h"
#include "../../ui/checkbox/cyber_checkbox.h"
#include "../../app/wavelength_config.h"

class ShortcutsSettingsWidget;
class SystemOverrideManager;
class SnakeGameLayer;
class TypingTestLayer;
class VoiceRecognitionLayer;
class RetinaScanLayer;
class SecurityQuestionLayer;
class SecurityCodeLayer;
class HandprintLayer;
class FingerprintLayer;
class AppearanceSettingsWidget;
class WavelengthSettingsWidget;
class NetworkSettingsWidget;
class QSpinBox;
class QLabel;
class QTimer;
class QPushButton;

class SettingsView final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsView(QWidget *parent = nullptr);
    ~SettingsView() override;

    void SetDebugMode(bool enabled);

signals:
    void backToMainView();
    void settingsChanged();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void SaveSettings();
    void RestoreDefaults();
    void SwitchToTab(int tab_index);
    void HandleBackButton();

private:
    void LoadSettingsFromRegistry() const;
    void SetupUi();
    void SetupClassifiedTab();
    void SetupNextSecurityLayer();
    void ResetSecurityLayers();
    void CreateHeaderPanel();

    WavelengthConfig *config_;
    QStackedWidget *tab_content_;
    QLabel *title_label_;
    QLabel *session_label_;
    QLabel *time_label_;
    QWidget *tab_bar_;
    QList<TabButton*> tab_buttons_;

    WavelengthSettingsWidget* wavelength_tab_widget_;
    AppearanceSettingsWidget* appearance_tab_widget_;
    NetworkSettingsWidget* advanced_tab_widget_;
    ShortcutsSettingsWidget* shortcuts_tab_widget_;

    // Przyciski akcji
    CyberButton *save_button_;
    CyberButton *defaults_button_;
    CyberButton *back_button_;

    QTimer *refresh_timer_;

    // Warstwy bezpieczeństwa (bez zmian)
    FingerprintLayer* fingerprint_layer_;
    HandprintLayer* handprint_layer_;
    SecurityCodeLayer* security_code_layer_;
    SecurityQuestionLayer* security_question_layer_;
    RetinaScanLayer* retina_scan_layer_;
    VoiceRecognitionLayer* voice_recognition_layer_;
    TypingTestLayer* typing_test_layer_;
    SnakeGameLayer* snake_game_layer_;
    QWidget* access_granted_widget_;
    QStackedWidget* security_layers_stack_;

    enum SecurityLayerIndex {
        FingerprintIndex = 0,
        HandprintIndex,
        SecurityCodeIndex,
        SecurityQuestionIndex,
        RetinaScanIndex,
        VoiceRecognitionIndex,
        TypingTestIndex,
        SnakeGameIndex,
        AccessGrantedIndex
    };

    SecurityLayerIndex current_layer_index_;
    bool debug_mode_enabled_;

    QWidget* classified_features_widget_;
    QPushButton* override_button_;
    SystemOverrideManager* system_override_manager_;
};

#endif // SETTINGS_VIEW_H