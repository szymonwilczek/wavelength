#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QTimer>

#include "../ui/button/cyber_button.h"
#include "../ui/checkbox/cyber_checkbox.h"
#include "../ui/input/cyber_line_edit.h"
#include "../util/wavelength_config.h"


class SettingsView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double glitchIntensity READ glitchIntensity WRITE setGlitchIntensity)
    Q_PROPERTY(double cornerGlowProgress READ cornerGlowProgress WRITE setCornerGlowProgress)

public:
    explicit SettingsView(QWidget *parent = nullptr);
    ~SettingsView() override;

    double glitchIntensity() const { return m_glitchIntensity; }
    void setGlitchIntensity(double intensity) {
        m_glitchIntensity = intensity;
        update();
        if (intensity > 0.4) regenerateGlitchLines();
    }
    
    double cornerGlowProgress() const { return m_cornerGlowProgress; }
    void setCornerGlowProgress(double progress) {
        m_cornerGlowProgress = progress;
        update();
    }

signals:
    void backToMainView();
    void settingsChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void saveSettings();
    void restoreDefaults();
    void applyTheme(const QString &themeName);

private:
    void regenerateGlitchLines();
    void loadSettingsFromRegistry();
    void setupUi();
    void setupUserTab(QWidget *tab);
    void setupServerTab(QWidget *tab);
    void setupAppearanceTab(QWidget *tab);
    void setupNetworkTab(QWidget *tab);
    void setupAdvancedTab(QWidget *tab);
    
    void createHeaderPanel();
    void animateEntrance();

    WavelengthConfig *m_config;
    QTabWidget *m_tabWidget;
    QLabel *m_titleLabel;
    QLabel *m_sessionLabel;
    QLabel *m_timeLabel;
    
    // Kontrolki do edycji ustawień - User
    CyberLineEdit *m_userNameEdit;
    
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
    QList<int> m_glitchLines;
    double m_glitchIntensity = 0.0;
    double m_cornerGlowProgress = 0.0;
};

#endif // SETTINGS_VIEW_H