#include "settings_view.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QRandomGenerator>
#include <QApplication>
#include <QStyle>
#include <QDateTime>

#include "../ui/button/cyber_button.h"
#include "../ui/checkbox/cyber_checkbox.h"
#include "../ui/input/cyber_line_edit.h"
#include "../util/wavelength_config.h"

SettingsView::SettingsView(QWidget *parent)
    : QWidget(parent),
      m_config(WavelengthConfig::getInstance())
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("SettingsView { background-color: #101820; }");
    
    m_glitchLines = QList<int>();
    
    setupUi();
    
    // Inicjalizacja timera dla efektów
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(33);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        if (m_glitchIntensity > 0.01) {
            update();
        }
        
        // Aktualizacja czasu co sekundę
        static QElapsedTimer timer;
        if (!timer.isValid() || timer.elapsed() > 1000) {
            QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
            m_timeLabel->setText(QString("TS: %1").arg(timestamp));
            timer.restart();
        }
    });
    m_refreshTimer->start();
}

SettingsView::~SettingsView() {
    if (m_refreshTimer) {
        m_refreshTimer->stop();
        delete m_refreshTimer;
        m_refreshTimer = nullptr;
    }
}

void SettingsView::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(15);
    
    createHeaderPanel();
    
    // Zakładki ustawień
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #005577; background-color: rgba(10, 25, 40, 180); border-radius: 5px; }"
        "QTabWidget::tab-bar { left: 5px; }"
        "QTabBar::tab { background-color: rgba(0, 30, 50, 180); color: #00aaff; padding: 10px 20px; margin-right: 2px; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background-color: rgba(0, 50, 80, 200); color: #ffffff; }"
        "QTabBar::tab:hover:!selected { background-color: rgba(0, 40, 60, 200); }"
    );
    
    // Zakładka User
    QWidget *userTab = new QWidget();
    setupUserTab(userTab);
    m_tabWidget->addTab(userTab, "User");
    
    // Zakładka Server
    QWidget *serverTab = new QWidget();
    setupServerTab(serverTab);
    m_tabWidget->addTab(serverTab, "Server");
    
    // Zakładka Appearance
    QWidget *appearanceTab = new QWidget();
    setupAppearanceTab(appearanceTab);
    m_tabWidget->addTab(appearanceTab, "Appearance");
    
    // Zakładka Network
    QWidget *networkTab = new QWidget();
    setupNetworkTab(networkTab);
    m_tabWidget->addTab(networkTab, "Network");
    
    // Zakładka Advanced
    QWidget *advancedTab = new QWidget();
    setupAdvancedTab(advancedTab);
    m_tabWidget->addTab(advancedTab, "Advanced");
    
    mainLayout->addWidget(m_tabWidget);
    
    // Panel przycisków
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);
    
    m_backButton = new CyberButton("BACK", this, false);
    m_backButton->setFixedHeight(40);
    
    m_defaultsButton = new CyberButton("RESTORE DEFAULTS", this, false);
    m_defaultsButton->setFixedHeight(40);
    
    m_saveButton = new CyberButton("SAVE SETTINGS", this, true);
    m_saveButton->setFixedHeight(40);
    
    buttonLayout->addWidget(m_backButton, 1);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_defaultsButton, 1);
    buttonLayout->addWidget(m_saveButton, 2);
    
    mainLayout->addLayout(buttonLayout);
    
    // Połączenia
    connect(m_backButton, &CyberButton::clicked, this, &SettingsView::backToMainView);
    connect(m_saveButton, &CyberButton::clicked, this, &SettingsView::saveSettings);
    connect(m_defaultsButton, &CyberButton::clicked, this, &SettingsView::restoreDefaults);
    
    // Początkowe ukrycie zakładek (przed animacją)
    m_tabWidget->setVisible(false);
}

void SettingsView::setupUserTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    QLabel *infoLabel = new QLabel("Configure your user profile settings", tab);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(infoLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(15);
    
    m_userNameEdit = new CyberLineEdit(tab);
    m_userNameEdit->setPlaceholderText("Enter your username");
    formLayout->addRow("Username:", m_userNameEdit);
    
    // Można dodać więcej pól dotyczących użytkownika (np. awatar, preferencje)
    
    layout->addLayout(formLayout);
    layout->addStretch();
}

void SettingsView::setupServerTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    QLabel *infoLabel = new QLabel("Configure server connection settings", tab);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(infoLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(15);
    
    m_serverAddressEdit = new CyberLineEdit(tab);
    m_serverAddressEdit->setPlaceholderText("Enter server address");
    formLayout->addRow("Server Address:", m_serverAddressEdit);
    
    m_serverPortEdit = new QSpinBox(tab);
    m_serverPortEdit->setRange(1, 65535);
    m_serverPortEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
        "QSpinBox::up-button, QSpinBox::down-button { background-color: rgba(0, 150, 220, 100); }"
    );
    formLayout->addRow("Server Port:", m_serverPortEdit);
    
    layout->addLayout(formLayout);
    layout->addStretch();
}

void SettingsView::setupAppearanceTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    QLabel *infoLabel = new QLabel("Configure application appearance", tab);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(infoLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(15);
    
    m_themeComboBox = new QComboBox(tab);
    m_themeComboBox->addItem("Light");
    m_themeComboBox->addItem("Dark");
    m_themeComboBox->addItem("Cyberpunk");
    m_themeComboBox->setStyleSheet(
        "QComboBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
        "QComboBox QAbstractItemView { color: #00eeff; background-color: rgba(10, 25, 40, 220); selection-background-color: rgba(0, 150, 220, 150); }"
    );
    formLayout->addRow("Theme:", m_themeComboBox);
    
    m_animationDurationEdit = new QSpinBox(tab);
    m_animationDurationEdit->setRange(100, 2000);
    m_animationDurationEdit->setSingleStep(50);
    m_animationDurationEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
        "QSpinBox::up-button, QSpinBox::down-button { background-color: rgba(0, 150, 220, 100); }"
    );
    formLayout->addRow("Animation Duration (ms):", m_animationDurationEdit);
    
    // Podgląd motywu
    QGroupBox *previewBox = new QGroupBox("Theme Preview", tab);
    previewBox->setStyleSheet("QGroupBox { color: #00ccff; background-color: transparent; border: 1px solid #005577; border-radius: 5px; padding: 10px; }");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewBox);
    
    QLabel *previewLabel = new QLabel("This is a preview of the selected theme.", previewBox);
    previewLabel->setStyleSheet("color: #ffffff; background-color: transparent;");
    previewLayout->addWidget(previewLabel);
    
    QHBoxLayout *demoWidgetsLayout = new QHBoxLayout();
    
    CyberButton *demoButton = new CyberButton("Demo Button", previewBox);
    demoWidgetsLayout->addWidget(demoButton);
    
    CyberLineEdit *demoLineEdit = new CyberLineEdit(previewBox);
    demoLineEdit->setPlaceholderText("Demo Input");
    demoWidgetsLayout->addWidget(demoLineEdit);
    
    previewLayout->addLayout(demoWidgetsLayout);
    
    layout->addLayout(formLayout);
    layout->addSpacing(20);
    layout->addWidget(previewBox);
    layout->addStretch();
    
    // Połączenie sygnału zmiany motywu
    connect(m_themeComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &SettingsView::applyTheme);
}

void SettingsView::setupNetworkTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    QLabel *infoLabel = new QLabel("Configure network connection parameters", tab);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(infoLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(15);
    
    m_connectionTimeoutEdit = new QSpinBox(tab);
    m_connectionTimeoutEdit->setRange(1000, 60000);
    m_connectionTimeoutEdit->setSingleStep(500);
    m_connectionTimeoutEdit->setSuffix(" ms");
    m_connectionTimeoutEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
    );
    formLayout->addRow("Connection Timeout:", m_connectionTimeoutEdit);
    
    m_keepAliveIntervalEdit = new QSpinBox(tab);
    m_keepAliveIntervalEdit->setRange(1000, 60000);
    m_keepAliveIntervalEdit->setSingleStep(500);
    m_keepAliveIntervalEdit->setSuffix(" ms");
    m_keepAliveIntervalEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
    );
    formLayout->addRow("Keep Alive Interval:", m_keepAliveIntervalEdit);
    
    m_maxReconnectAttemptsEdit = new QSpinBox(tab);
    m_maxReconnectAttemptsEdit->setRange(0, 20);
    m_maxReconnectAttemptsEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
    );
    formLayout->addRow("Max Reconnect Attempts:", m_maxReconnectAttemptsEdit);
    
    layout->addLayout(formLayout);
    layout->addStretch();
}

void SettingsView::setupAdvancedTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);
    
    QLabel *infoLabel = new QLabel("Advanced application settings - modify with caution", tab);
    infoLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(infoLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(15);
    
    m_debugModeCheckBox = new CyberCheckBox("Enable detailed logging and debugging features", tab);
    formLayout->addRow("Debug Mode:", m_debugModeCheckBox);
    
    m_chatHistorySizeEdit = new QSpinBox(tab);
    m_chatHistorySizeEdit->setRange(50, 1000);
    m_chatHistorySizeEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
    );
    formLayout->addRow("Chat History Size:", m_chatHistorySizeEdit);
    
    m_processedMessageIdsEdit = new QSpinBox(tab);
    m_processedMessageIdsEdit->setRange(50, 1000);
    m_processedMessageIdsEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
    );
    formLayout->addRow("Max Processed Message IDs:", m_processedMessageIdsEdit);
    
    m_sentMessageCacheSizeEdit = new QSpinBox(tab);
    m_sentMessageCacheSizeEdit->setRange(10, 500);
    m_sentMessageCacheSizeEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
    );
    formLayout->addRow("Sent Message Cache Size:", m_sentMessageCacheSizeEdit);
    
    m_maxRecentWavelengthEdit = new QSpinBox(tab);
    m_maxRecentWavelengthEdit->setRange(1, 30);
    m_maxRecentWavelengthEdit->setStyleSheet(
        "QSpinBox { color: #00eeff; background-color: rgba(10, 25, 40, 180); border: 1px solid #00aaff; padding: 5px; }"
    );
    formLayout->addRow("Max Recent Wavelength:", m_maxRecentWavelengthEdit);
    
    layout->addLayout(formLayout);
    layout->addStretch();
}

void SettingsView::createHeaderPanel() {
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);
    
    // Tytuł
    m_titleLabel = new QLabel("SYSTEM SETTINGS", this);
    m_titleLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 15pt; letter-spacing: 2px;");
    headerLayout->addWidget(m_titleLabel);
    
    headerLayout->addStretch();
    
    // Panel informacyjny z ID sesji i czasem
    QString sessionId = QString("%1-%2")
        .arg(QRandomGenerator::global()->bounded(1000, 9999))
        .arg(QRandomGenerator::global()->bounded(10000, 99999));
    
    m_sessionLabel = new QLabel(QString("SESSION_ID: %1").arg(sessionId), this);
    m_sessionLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");
    headerLayout->addWidget(m_sessionLabel);
    
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_timeLabel = new QLabel(QString("TS: %1").arg(timestamp), this);
    m_timeLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");
    headerLayout->addWidget(m_timeLabel);
    
    dynamic_cast<QVBoxLayout*>(layout())->addLayout(headerLayout);
}

void SettingsView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    loadSettingsFromRegistry();
    animateEntrance();
}

void SettingsView::animateEntrance() {
    // Animacja efektu narożników
    QPropertyAnimation *cornerGlowAnim = new QPropertyAnimation(this, "cornerGlowProgress");
    cornerGlowAnim->setDuration(1200);
    cornerGlowAnim->setStartValue(0.0);
    cornerGlowAnim->setEndValue(1.0);
    cornerGlowAnim->setEasingCurve(QEasingCurve::OutQuad);
    
    // Animacja glitchu
    QPropertyAnimation *glitchAnim = new QPropertyAnimation(this, "glitchIntensity");
    glitchAnim->setDuration(800);
    glitchAnim->setStartValue(0.6);
    glitchAnim->setEndValue(0.0);
    glitchAnim->setEasingCurve(QEasingCurve::OutExpo);
    
    // Animacja pokazania zakładek
    QTimer::singleShot(300, this, [this]() {
        m_tabWidget->setVisible(true);
        
        // Ustaw przezroczystość na 0
        auto *opacityEffect = new QGraphicsOpacityEffect(m_tabWidget);
        opacityEffect->setOpacity(0.0);
        m_tabWidget->setGraphicsEffect(opacityEffect);
        
        // Animuj przezroczystość do 1
        QPropertyAnimation *opacityAnim = new QPropertyAnimation(opacityEffect, "opacity");
        opacityAnim->setDuration(500);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);
        opacityAnim->start(QPropertyAnimation::DeleteWhenStopped);
    });
    
    // Grupa animacji
    QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
    animGroup->addAnimation(cornerGlowAnim);
    animGroup->addAnimation(glitchAnim);
    animGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

void SettingsView::loadSettingsFromRegistry() {
    // User
    m_userNameEdit->setText(m_config->getUserName());
    
    // Server
    m_serverAddressEdit->setText(m_config->getRelayServerAddress());
    m_serverPortEdit->setValue(m_config->getRelayServerPort());
    
    // Appearance
    QString theme = m_config->getApplicationTheme();
    if (theme == "light") {
        m_themeComboBox->setCurrentIndex(0);
    } else if (theme == "dark") {
        m_themeComboBox->setCurrentIndex(1);
    } else if (theme == "cyberpunk") {
        m_themeComboBox->setCurrentIndex(2);
    }
    
    // Network
    m_connectionTimeoutEdit->setValue(m_config->getConnectionTimeout());
    m_keepAliveIntervalEdit->setValue(m_config->getKeepAliveInterval());
    m_maxReconnectAttemptsEdit->setValue(m_config->getMaxReconnectAttempts());
    
    // Advanced
    m_debugModeCheckBox->setChecked(m_config->isDebugMode());
    m_chatHistorySizeEdit->setValue(m_config->getMaxChatHistorySize());
    m_processedMessageIdsEdit->setValue(m_config->getMaxProcessedMessageIds());
    m_sentMessageCacheSizeEdit->setValue(m_config->getMaxSentMessageCacheSize());
    m_maxRecentWavelengthEdit->setValue(m_config->getMaxRecentWavelength());
}

void SettingsView::saveSettings() {
    // User
    m_config->setUserName(m_userNameEdit->text());
    
    // Server
    m_config->setRelayServerAddress(m_serverAddressEdit->text());
    m_config->setRelayServerPort(m_serverPortEdit->value());
    
    // Appearance
    QString theme;
    switch (m_themeComboBox->currentIndex()) {
        case 0: theme = "light"; break;
        case 1: theme = "dark"; break;
        case 2: theme = "cyberpunk"; break;
        default: theme = "dark";
    }
    m_config->setApplicationTheme(theme);
    
    // Network
    m_config->setConnectionTimeout(m_connectionTimeoutEdit->value());
    m_config->setKeepAliveInterval(m_keepAliveIntervalEdit->value());
    m_config->setMaxReconnectAttempts(m_maxReconnectAttemptsEdit->value());
    
    // Advanced
    m_config->setDebugMode(m_debugModeCheckBox->isChecked());
    m_config->setMaxChatHistorySize(m_chatHistorySizeEdit->value());
    m_config->setMaxProcessedMessageIds(m_processedMessageIdsEdit->value());
    m_config->setMaxSentMessageCacheSize(m_sentMessageCacheSizeEdit->value());
    m_config->setMaxRecentWavelength(m_maxRecentWavelengthEdit->value());
    
    // Zapisanie ustawień
    m_config->saveSettings();
    
    QMessageBox::information(this, "Settings Saved", "Settings have been successfully saved to registry.");
    
    emit settingsChanged(); // Informujemy o zmianie ustawień
}

void SettingsView::restoreDefaults() {
    if (QMessageBox::question(this, "Restore Defaults", 
                             "Are you sure you want to restore all settings to default values?",
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_config->restoreDefaults();
        loadSettingsFromRegistry();
    }
}

void SettingsView::applyTheme(const QString &themeName) {
    // Podgląd wybranego motywu (można uzupełnić później)
}

void SettingsView::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Rysowanie efektów wizualnych (glitch, narożniki itp.)
    
    // Efekt glitch
    if (m_glitchIntensity > 0.01) {
        painter.setPen(QPen(QColor(255, 50, 120, 150 * m_glitchIntensity), 1));
        
        for (int i = 0; i < m_glitchLines.size(); i++) {
            int y = m_glitchLines[i];
            int offset = QRandomGenerator::global()->bounded(5, 15) * m_glitchIntensity;
            int width = QRandomGenerator::global()->bounded(30, 80) * m_glitchIntensity;
            
            if (QRandomGenerator::global()->bounded(2) == 0) {
                painter.drawLine(0, y, width, y + offset);
            } else {
                painter.drawLine(this->width() - width, y, this->width(), y + offset);
            }
        }
    }
    
    // Podświetlanie rogów (opcjonalnie)
    if (m_cornerGlowProgress > 0.0) {
        int clipSize = 20; // rozmiar ścięcia
        QColor cornerColor(0, 220, 255, 150);
        painter.setPen(QPen(cornerColor, 2));
        
        double step = 1.0 / 4;
        
        if (m_cornerGlowProgress >= step * 0) {
            painter.drawLine(0, clipSize * 1.5, 0, clipSize);
            painter.drawLine(0, clipSize, clipSize, 0);
            painter.drawLine(clipSize, 0, clipSize * 1.5, 0);
        }
        
        if (m_cornerGlowProgress >= step * 1) {
            painter.drawLine(width() - clipSize * 1.5, 0, width() - clipSize, 0);
            painter.drawLine(width() - clipSize, 0, width(), clipSize);
            painter.drawLine(width(), clipSize, width(), clipSize * 1.5);
        }
        
        if (m_cornerGlowProgress >= step * 2) {
            painter.drawLine(width(), height() - clipSize * 1.5, width(), height() - clipSize);
            painter.drawLine(width(), height() - clipSize, width() - clipSize, height());
            painter.drawLine(width() - clipSize, height(), width() - clipSize * 1.5, height());
        }
        
        if (m_cornerGlowProgress >= step * 3) {
            painter.drawLine(clipSize * 1.5, height(), clipSize, height());
            painter.drawLine(clipSize, height(), 0, height() - clipSize);
            painter.drawLine(0, height() - clipSize, 0, height() - clipSize * 1.5);
        }
    }
}

void SettingsView::regenerateGlitchLines() {
    static QElapsedTimer lastGlitchUpdate;
    if (!lastGlitchUpdate.isValid() || lastGlitchUpdate.elapsed() > 100) {
        m_glitchLines.clear();
        int glitchCount = 3 + static_cast<int>(m_glitchIntensity * 7);
        for (int i = 0; i < glitchCount; i++) {
            m_glitchLines.append(QRandomGenerator::global()->bounded(height()));
        }
        lastGlitchUpdate.restart();
    }
}