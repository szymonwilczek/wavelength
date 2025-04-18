#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H

#include <QComboBox>
#include <QSpinBox>
#include <QStackedWidget>
#include <QDateTime>

#include "../ui/button/cyber_button.h"
#include "../ui/button/tab_button.h"
#include "../ui/checkbox/cyber_checkbox.h"
#include "../ui/input/cyber_line_edit.h"
#include "../util/wavelength_config.h"
#include "settings/classified/system_override_manager.h"
#include "settings/layers/code/security_code_layer.h"
#include "settings/layers/fingerprint/fingerprint_layer.h"
#include "settings/layers/handprint/handprint_layer.h"
#include "settings/layers/question/security_question_layer.h"
#include "settings/layers/retina_scan/retina_scan_layer.h"
#include "settings/layers/snake_game/snake_game_layer.h"
#include "settings/layers/typing_test/typing_test_layer.h"
#include "settings/layers/voice_recognition/voice_recognition_layer.h"

class SettingsView : public QWidget {
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
    void loadSettingsFromRegistry();
    void setupUi();
    void setupServerTab();
    void setupAppearanceTab();
    void setupNetworkTab();
    void setupAdvancedTab();
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

    // Kontrolki do edycji ustawień - Server
    CyberLineEdit *m_serverAddressEdit;
    QSpinBox *m_serverPortEdit;

    // Kontrolki do edycji ustawień - Appearance
    QComboBox *m_themeComboBox;
    QSpinBox *m_animationDurationEdit;

    // Kontrolki do edycji ustawień - Network
    QSpinBox *m_connectionTimeoutEdit;
    QSpinBox *m_keepAliveIntervalEdit;
    QSpinBox *m_maxReconnectAttemptsEdit;

    // Kontrolki do edycji ustawień - Advanced
    CyberCheckBox *m_debugModeCheckBox;
    QSpinBox *m_chatHistorySizeEdit;
    QSpinBox *m_processedMessageIdsEdit;
    QSpinBox *m_sentMessageCacheSizeEdit;
    QSpinBox *m_maxRecentWavelengthEdit;

    // Przyciski akcji
    CyberButton *m_saveButton;
    CyberButton *m_defaultsButton;
    CyberButton *m_backButton;

    QTimer *m_refreshTimer;

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