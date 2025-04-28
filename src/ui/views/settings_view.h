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

    void setDebugMode(bool enabled);

signals:
    void backToMainView();
    void settingsChanged();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void saveSettings();
    void restoreDefaults();
    void switchToTab(int tabIndex);
    void handleBackButton();

private:
    void loadSettingsFromRegistry() const;
    void setupUi();
    void setupClassifiedTab();
    void setupNextSecurityLayer();
    void resetSecurityLayers();
    void createHeaderPanel();

    WavelengthConfig *m_config;
    QStackedWidget *m_tabContent;
    QLabel *m_titleLabel;
    QLabel *m_sessionLabel;
    QLabel *m_timeLabel;
    QWidget *m_tabBar;
    QList<TabButton*> m_tabButtons;

    WavelengthSettingsWidget* m_wavelengthTabWidget;
    AppearanceSettingsWidget* m_appearanceTabWidget;
    NetworkSettingsWidget* m_advancedTabWidget;
    ShortcutsSettingsWidget* m_shortcutsTabWidget;

    // Przyciski akcji
    CyberButton *m_saveButton;
    CyberButton *m_defaultsButton;
    CyberButton *m_backButton;

    QTimer *m_refreshTimer;

    // Warstwy bezpieczeństwa (bez zmian)
    FingerprintLayer* m_fingerprintLayer;
    HandprintLayer* m_handprintLayer;
    SecurityCodeLayer* m_securityCodeLayer;
    SecurityQuestionLayer* m_securityQuestionLayer;
    RetinaScanLayer* m_retinaScanLayer;
    VoiceRecognitionLayer* m_voiceRecognitionLayer;
    TypingTestLayer* m_typingTestLayer;
    SnakeGameLayer* m_snakeGameLayer;
    QWidget* m_accessGrantedWidget;
    QStackedWidget* m_securityLayersStack;

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

    SecurityLayerIndex m_currentLayerIndex;
    bool m_debugModeEnabled;

    QWidget* m_classifiedFeaturesWidget;
    QPushButton* m_overrideButton;
    SystemOverrideManager* m_systemOverrideManager;
};

#endif // SETTINGS_VIEW_H