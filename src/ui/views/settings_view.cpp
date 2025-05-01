#include "settings_view.h"

#include <QHBoxLayout>
#include <QFormLayout>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QRandomGenerator>
#include <QPushButton>

#include "../../ui/settings/tabs/classified/components/system_override_manager.h"
#include "../../ui/settings/tabs/appearance/appearance_settings_widget.h"
#include "../../ui/settings/tabs/wavelength/wavelength_settings_widget.h"
#include "../../ui/settings/tabs/network/network_settings_widget.h"

#include "../../ui/settings/tabs/classified/layers/fingerprint/fingerprint_layer.h"
#include "../../ui/settings/tabs/classified/layers/handprint/handprint_layer.h"
#include "../../ui/settings/tabs/classified/layers/code/security_code_layer.h"
#include "../../ui/settings/tabs/classified/layers/question/security_question_layer.h"
#include "../../ui/settings/tabs/classified/layers/retina_scan/retina_scan_layer.h"
#include "../../ui/settings/tabs/classified/layers/voice_recognition/voice_recognition_layer.h"
#include "../../ui/settings/tabs/classified/layers/typing_test/typing_test_layer.h"
#include "../../ui/settings/tabs/classified/layers/snake_game/snake_game_layer.h"

#include "../../ui/checkbox/cyber_checkbox.h"
#include "../../app/managers/shortcut_manager.h"
#include "../../ui/settings/tabs/shortcuts/shortcuts_settings_widget.h"

SettingsView::SettingsView(QWidget *parent)
    : QWidget(parent),
m_config(WavelengthConfig::GetInstance()),
m_tabContent(nullptr),
m_titleLabel(nullptr),
m_sessionLabel(nullptr),
m_timeLabel(nullptr),
m_tabBar(nullptr),
m_wavelengthTabWidget(nullptr),
m_appearanceTabWidget(nullptr),
m_advancedTabWidget(nullptr),
m_saveButton(nullptr),
m_defaultsButton(nullptr),
m_backButton(nullptr),
m_refreshTimer(nullptr), // Zmieniono inicjalizację na false zgodnie z komentarzem w kodzie
m_fingerprintLayer(nullptr), // Dodano inicjalizację wskaźników
m_handprintLayer(nullptr),
m_securityCodeLayer(nullptr), // <<< Dodano inicjalizację
m_securityQuestionLayer(nullptr),
m_retinaScanLayer(nullptr),
m_voiceRecognitionLayer(nullptr),
m_typingTestLayer(nullptr),
m_snakeGameLayer(nullptr), // Nadal inicjalizowany, chociaż może nie być używany
m_accessGrantedWidget(nullptr),
m_securityLayersStack(nullptr),
m_currentLayerIndex(FingerprintIndex),
m_debugModeEnabled(true),
m_classifiedFeaturesWidget(nullptr),
m_overrideButton(nullptr),
m_systemOverrideManager(nullptr)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("SettingsView { background-color: #101820; }");

    setupUi();

    // Timer do aktualizacji czasu
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        if(m_timeLabel) m_timeLabel->setText(QString("TS: %1").arg(timestamp));
    });
    m_refreshTimer->start();

    m_systemOverrideManager = new SystemOverrideManager(this);

    connect(m_systemOverrideManager, &SystemOverrideManager::overrideFinished, this, [this](){
        qDebug() << "Override sequence finished signal received in SettingsView.";
        if (m_overrideButton) {
            m_overrideButton->setEnabled(true);
            m_overrideButton->setText("CAUTION: SYSTEM OVERRIDE");
        }
        // Można dodać inne akcje po zakończeniu override
    });
}

void SettingsView::setDebugMode(const bool enabled) {
    if (m_debugModeEnabled != enabled) {
        m_debugModeEnabled = enabled;
        qDebug() << "SettingsView Debug Mode:" << (enabled ? "ENABLED" : "DISABLED");
        // Po zmianie trybu debugowania, zresetuj stan zakładki CLASSIFIED
        resetSecurityLayers();
    }
}

SettingsView::~SettingsView() {
    // Nie trzeba usuwać widgetów-dzieci, Qt zrobi to samo
    // Wystarczy zatrzymać i usunąć timer
    if (m_refreshTimer) {
        m_refreshTimer->stop();
        // delete m_refreshTimer; // Qt zarządza pamięcią dzieci, nie usuwaj ręcznie
        // m_refreshTimer = nullptr;
    }
}

void SettingsView::setupUi() {
    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(15);

    createHeaderPanel();

    // Panel zakładek (przyciski zamiast QTabWidget)
    m_tabBar = new QWidget(this);
    const auto tabLayout = new QHBoxLayout(m_tabBar);
    tabLayout->setContentsMargins(0, 5, 0, 10);
    tabLayout->setSpacing(0);
    tabLayout->addStretch(1);

    // Tworzenie przycisków zakładek
    // Zmieniono "Performance" na "Network"
    QStringList tabNames = {"Wavelength", "Appearance", "Network","Shortcuts", "CLASSIFIED"};

    for (int i = 0; i < tabNames.size(); i++) {
        auto btn = new TabButton(tabNames[i], m_tabBar);

        // Specjalne formatowanie dla zakładki CLASSIFIED
        if (tabNames[i] == "CLASSIFIED") {
            btn->setStyleSheet(
                "TabButton {"
                "  color: #ff3333;"
                "  background-color: transparent;"
                "  border: none;"
                "  font-family: 'Consolas';"
                "  font-size: 11pt;"
                "  font-weight: bold;"
                "  padding: 5px 15px;"
                "  margin: 0px 10px;"
                "  text-align: center;"
                "}"
                "TabButton:hover {"
                "  color: #ff6666;"
                "}"
                "TabButton:checked {"
                "  color: #ff0000;"
                "}"
            );
        }

        connect(btn, &QPushButton::clicked, this, [this, i](){ switchToTab(i); });
        tabLayout->addWidget(btn);
        m_tabButtons.append(btn);
    }

    tabLayout->addStretch(1);
    mainLayout->addWidget(m_tabBar);

    // Widget ze stosem zawartości zakładek
    m_tabContent = new QStackedWidget(this);
    m_tabContent->setStyleSheet(
        "QWidget { background-color: rgba(10, 25, 40, 180); border: 1px solid #005577; border-radius: 5px; }"
    );

    // Tworzenie zawartości dla każdej zakładki
    m_wavelengthTabWidget = new WavelengthSettingsWidget(m_tabContent);
    m_tabContent->addWidget(m_wavelengthTabWidget); // Index 0

    m_appearanceTabWidget = new AppearanceSettingsWidget(m_tabContent);
    m_tabContent->addWidget(m_appearanceTabWidget); // Index 1

    // Usunięto wywołanie setupNetworkTab();
    // Dodano tworzenie AdvancedSettingsWidget (zakładka Network)
    m_advancedTabWidget = new NetworkSettingsWidget(m_tabContent); // <<< Utworzenie nowej zakładki
    m_tabContent->addWidget(m_advancedTabWidget); // Index 2

    m_shortcutsTabWidget = new ShortcutsSettingsWidget(m_tabContent);
    m_tabContent->addWidget(m_shortcutsTabWidget); // Index 3

    setupClassifiedTab(); // Tworzy zakładkę "CLASSIFIED" - Index 4

    mainLayout->addWidget(m_tabContent, 1);

    // Panel przycisków (bez zmian)
    const auto buttonLayout = new QHBoxLayout();
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

    // Połączenia (bez zmian)
    connect(m_backButton, &CyberButton::clicked, this, &SettingsView::handleBackButton);
    connect(m_saveButton, &CyberButton::clicked, this, &SettingsView::saveSettings);
    connect(m_defaultsButton, &CyberButton::clicked, this, &SettingsView::restoreDefaults);

    // Domyślnie aktywna pierwsza zakładka
    if (!m_tabButtons.isEmpty()) {
        m_tabButtons[0]->setActive(true);
        m_tabButtons[0]->setChecked(true);
    }
    m_tabContent->setCurrentIndex(0);
}

void SettingsView::setupClassifiedTab() {
    // Implementacja bez zmian...
    const auto tab = new QWidget(m_tabContent);
    const auto layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    const auto infoLabel = new QLabel("CLASSIFIED INFORMATION - SECURITY VERIFICATION REQUIRED", tab);
    infoLabel->setStyleSheet("color: #ff3333; background-color: transparent; font-family: Consolas; font-size: 12pt; font-weight: bold;");
    layout->addWidget(infoLabel);

    const auto warningLabel = new QLabel("Authorization required to access classified settings.\nMultiple security layers will be enforced.", tab);
    warningLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(warningLabel);

    m_securityLayersStack = new QStackedWidget(tab);
    layout->addWidget(m_securityLayersStack, 1);

    m_fingerprintLayer = new FingerprintLayer(m_securityLayersStack);
    m_handprintLayer = new HandprintLayer(m_securityLayersStack);
    m_securityCodeLayer = new SecurityCodeLayer(m_securityLayersStack);
    m_securityQuestionLayer = new SecurityQuestionLayer(m_securityLayersStack);
    m_retinaScanLayer = new RetinaScanLayer(m_securityLayersStack);
    m_voiceRecognitionLayer = new VoiceRecognitionLayer(m_securityLayersStack);
    m_typingTestLayer = new TypingTestLayer(m_securityLayersStack);
    m_snakeGameLayer = new SnakeGameLayer(m_securityLayersStack);

    m_classifiedFeaturesWidget = new QWidget(m_securityLayersStack);
    const auto featuresLayout = new QVBoxLayout(m_classifiedFeaturesWidget);
    featuresLayout->setAlignment(Qt::AlignCenter);
    featuresLayout->setSpacing(20);

    const auto featuresTitle = new QLabel("CLASSIFIED FEATURES UNLOCKED", m_classifiedFeaturesWidget);
    featuresTitle->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 14pt; font-weight: bold;");
    featuresTitle->setAlignment(Qt::AlignCenter);
    featuresLayout->addWidget(featuresTitle);

    const auto featuresDesc = new QLabel("You have bypassed all security layers.\nThe true potential is now available.", m_classifiedFeaturesWidget);
    featuresDesc->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    featuresDesc->setAlignment(Qt::AlignCenter);
    featuresLayout->addWidget(featuresDesc);

    const auto buttonContainer = new QWidget(m_classifiedFeaturesWidget);
    const auto buttonLayoutClassified = new QHBoxLayout(buttonContainer); // Zmieniono nazwę zmiennej
    buttonLayoutClassified->setSpacing(15);

    const auto waveSculptorButton = new QPushButton("Wave Sculptor", buttonContainer);
    waveSculptorButton->setMinimumHeight(40);
    waveSculptorButton->setStyleSheet(
        "QPushButton { background-color: #0077AA; border: 1px solid #00AAFF; padding: 10px; border-radius: 5px; color: #E0E0E0; font-family: Consolas; font-weight: bold; }"
        "QPushButton:hover { background-color: #0099CC; }"
        "QPushButton:pressed { background-color: #005588; }"
    );
    buttonLayoutClassified->addWidget(waveSculptorButton);

    const auto matrixVisionButton = new QPushButton("Matrix Vision (TBD)", buttonContainer);
    matrixVisionButton->setMinimumHeight(40);
    matrixVisionButton->setStyleSheet("QPushButton { background-color: #444; border: 1px solid #666; padding: 10px; border-radius: 5px; color: #999; }");
    matrixVisionButton->setEnabled(false);
    buttonLayoutClassified->addWidget(matrixVisionButton);

    m_overrideButton = new QPushButton("CAUTION: SYSTEM OVERRIDE", buttonContainer);
    m_overrideButton->setMinimumHeight(50);
    m_overrideButton->setMinimumWidth(250);
    m_overrideButton->setStyleSheet(
        "QPushButton {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #AA0000, stop:1 #660000);"
        "  border: 2px solid #FF3333;"
        "  border-radius: 8px;"
        "  color: #FFFFFF;"
        "  font-family: 'Consolas';"
        "  font-size: 12pt;"
        "  font-weight: bold;"
        "  padding: 10px;"
        "  text-transform: uppercase;"
        "}"
        "QPushButton:hover {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #CC0000, stop:1 #880000);"
        "  border-color: #FF6666;"
        "}"
        "QPushButton:pressed {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #880000, stop:1 #440000);"
        "  border-color: #DD0000;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #555;"
        "  border-color: #777;"
        "  color: #999;"
        "}"
    );
    m_overrideButton->setEnabled(true);
    connect(m_overrideButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Override button clicked.";
#ifdef Q_OS_WIN
        if (SystemOverrideManager::isRunningAsAdmin()) {
            qDebug() << "Already running as admin. Initiating sequence directly.";
            m_overrideButton->setEnabled(false);
            m_overrideButton->setText("OVERRIDE IN PROGRESS...");
            QMetaObject::invokeMethod(m_systemOverrideManager, "initiateOverrideSequence", Qt::QueuedConnection, Q_ARG(bool, false));
        } else {
            qDebug() << "Not running as admin. Attempting relaunch with elevation request...";
            QStringList args;
            args << "--run-override";
            if (SystemOverrideManager::relaunchAsAdmin(args)) {
                qDebug() << "Relaunch initiated. Closing current instance.";
                QApplication::quit();
            } else {
                QMessageBox::warning(this, "Elevation Failed", "Could not request administrator privileges.\nThe override sequence cannot start.");
            }
        }
#else
        QMessageBox::information(this, "System Override", "System override sequence initiated (OS specific elevation might be required).");
        m_overrideButton->setEnabled(false);
        m_overrideButton->setText("OVERRIDE IN PROGRESS...");
        QMetaObject::invokeMethod(m_systemOverrideManager, "initiateOverrideSequence", Qt::QueuedConnection, Q_ARG(bool, false));
#endif
    });
    buttonLayoutClassified->addWidget(m_overrideButton);

    featuresLayout->addWidget(buttonContainer, 0, Qt::AlignCenter);
    featuresLayout->addStretch();

    m_securityLayersStack->addWidget(m_fingerprintLayer);
    m_securityLayersStack->addWidget(m_handprintLayer);
    m_securityLayersStack->addWidget(m_securityCodeLayer);
    m_securityLayersStack->addWidget(m_securityQuestionLayer);
    m_securityLayersStack->addWidget(m_retinaScanLayer);
    m_securityLayersStack->addWidget(m_voiceRecognitionLayer);
    m_securityLayersStack->addWidget(m_typingTestLayer);
    m_securityLayersStack->addWidget(m_snakeGameLayer);
    m_securityLayersStack->addWidget(m_classifiedFeaturesWidget);

    // Połączenia warstw (bez zmian)
    connect(m_fingerprintLayer, &SecurityLayer::layerCompleted, this, [this]() {
        m_currentLayerIndex = HandprintIndex;
        m_securityLayersStack->setCurrentIndex(m_currentLayerIndex);
        m_handprintLayer->initialize();
    });
    connect(m_handprintLayer, &SecurityLayer::layerCompleted, this, [this]() {
        m_currentLayerIndex = SecurityCodeIndex;
        m_securityLayersStack->setCurrentIndex(m_currentLayerIndex);
        m_securityCodeLayer->initialize();
    });
    connect(m_securityCodeLayer, &SecurityLayer::layerCompleted, this, [this]() {
        m_currentLayerIndex = SecurityQuestionIndex;
        m_securityLayersStack->setCurrentIndex(m_currentLayerIndex);
        m_securityQuestionLayer->initialize();
    });
    connect(m_securityQuestionLayer, &SecurityLayer::layerCompleted, this, [this]() {
        m_currentLayerIndex = RetinaScanIndex;
        m_securityLayersStack->setCurrentIndex(m_currentLayerIndex);
        m_retinaScanLayer->initialize();
    });
    connect(m_retinaScanLayer, &SecurityLayer::layerCompleted, this, [this]() {
        m_currentLayerIndex = VoiceRecognitionIndex;
        m_securityLayersStack->setCurrentIndex(m_currentLayerIndex);
        m_voiceRecognitionLayer->initialize();
    });
    connect(m_voiceRecognitionLayer, &SecurityLayer::layerCompleted, this, [this]() {
        m_currentLayerIndex = TypingTestIndex;
        m_securityLayersStack->setCurrentIndex(m_currentLayerIndex);
        m_typingTestLayer->initialize();
    });
    connect(m_typingTestLayer, &SecurityLayer::layerCompleted, this, [this]() {
        m_currentLayerIndex = SnakeGameIndex;
        m_securityLayersStack->setCurrentIndex(m_currentLayerIndex);
        m_snakeGameLayer->initialize();
    });
    connect(m_snakeGameLayer, &SecurityLayer::layerCompleted, this, [this]() {
        int featuresIndex = m_securityLayersStack->indexOf(m_classifiedFeaturesWidget);
        if (featuresIndex != -1) {
             // Rzutowanie jest ok, jeśli AccessGrantedIndex jest ostatnim elementem enum PRZED dodaniem widgetu
             // Lepiej użyć bezpośrednio featuresIndex
             m_currentLayerIndex = static_cast<SecurityLayerIndex>(featuresIndex); // Lub jakaś wartość specjalna
             m_securityLayersStack->setCurrentIndex(featuresIndex);
             qDebug() << "All security layers passed. Showing classified features.";
        } else {
             qWarning() << "Could not find ClassifiedFeaturesWidget in the stack!";
             m_securityLayersStack->setCurrentIndex(m_securityLayersStack->count() - 1);
        }
    });

    layout->addStretch();
    m_tabContent->addWidget(tab); // Dodaj zakładkę do stosu

    resetSecurityLayers(); // Zresetuj na końcu
}

void SettingsView::setupNextSecurityLayer() {
    // Implementacja bez zmian...
    // Sprawdź, czy obecny indeks jest prawidłowy i czy nie jest to ostatnia warstwa przed dostępem
    // Użyjemy indeksu widgetu funkcji zamiast AccessGrantedIndex
    const int featuresIndex = m_securityLayersStack->indexOf(m_classifiedFeaturesWidget);
    if (featuresIndex == -1) {
        qWarning() << "Cannot setup next layer: ClassifiedFeaturesWidget not found.";
        return; // Błąd krytyczny
    }

    if (static_cast<int>(m_currentLayerIndex) < featuresIndex) {
         m_currentLayerIndex = static_cast<SecurityLayerIndex>(static_cast<int>(m_currentLayerIndex) + 1);
    } else {
        // Jeśli już jesteśmy na ClassifiedFeaturesWidget lub dalej (co nie powinno się zdarzyć), nie rób nic
        return;
    }

    // Przygotowanie następnego zabezpieczenia
    // Używamy indeksów enum, które odpowiadają indeksom widgetów dodanych wcześniej
    if (m_currentLayerIndex < featuresIndex) {
         m_securityLayersStack->setCurrentIndex(m_currentLayerIndex);
         // Wywołaj initialize dla odpowiedniej warstwy
         QWidget* currentWidget = m_securityLayersStack->widget(m_currentLayerIndex);
         if (const auto currentLayer = qobject_cast<SecurityLayer*>(currentWidget)) {
             currentLayer->initialize();
         } else {
             qWarning() << "Widget at index" << m_currentLayerIndex << "is not a SecurityLayer!";
         }
    } else if (m_currentLayerIndex == featuresIndex) {
         // Osiągnięto widget funkcji, nie ma initialize()
         m_securityLayersStack->setCurrentIndex(featuresIndex);
    }
}

void SettingsView::createHeaderPanel() {
    // Implementacja bez zmian...
    const auto headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);

    m_titleLabel = new QLabel("SYSTEM SETTINGS", this);
    m_titleLabel->setStyleSheet("color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 15pt; letter-spacing: 2px;");
    headerLayout->addWidget(m_titleLabel);

    headerLayout->addStretch();

    const QString sessionId = QString("%1-%2")
        .arg(QRandomGenerator::global()->bounded(1000, 9999))
        .arg(QRandomGenerator::global()->bounded(10000, 99999));

    m_sessionLabel = new QLabel(QString("SESSION_ID: %1").arg(sessionId), this);
    m_sessionLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");
    headerLayout->addWidget(m_sessionLabel);

    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_timeLabel = new QLabel(QString("TS: %1").arg(timestamp), this);
    m_timeLabel->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");
    headerLayout->addWidget(m_timeLabel);

    // Dodaj headerLayout do głównego layoutu (zakładając, że główny layout to QVBoxLayout)
    if (const auto mainVLayout = qobject_cast<QVBoxLayout*>(layout())) {
        mainVLayout->insertLayout(0, headerLayout); // Wstaw na górze
    } else {
        qWarning() << "Could not cast main layout to QVBoxLayout in createHeaderPanel";
        // Awaryjnie dodaj na końcu
        layout()->addItem(headerLayout);
    }
}

void SettingsView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    loadSettingsFromRegistry();
    resetSecurityLayers(); // Resetuj warstwy przy każdym pokazaniu widoku
}


void SettingsView::loadSettingsFromRegistry() const {
    // --- Wavelength ---
    if (m_wavelengthTabWidget) {
        m_wavelengthTabWidget->loadSettings();
    }
    // --- Appearance ---
    if (m_appearanceTabWidget) {
        m_appearanceTabWidget->loadSettings();
    }
    // --- Network (teraz w AdvancedSettingsWidget) ---
    if (m_advancedTabWidget) { // <<< Dodano ładowanie nowej zakładki
        m_advancedTabWidget->loadSettings();
    }

    if (m_shortcutsTabWidget) m_shortcutsTabWidget->loadSettings();
}

void SettingsView::saveSettings() {
    // --- Wavelength ---
    if (m_wavelengthTabWidget) {
        m_wavelengthTabWidget->saveSettings();
    }
    // --- Appearance ---
    if (m_appearanceTabWidget) {
        m_appearanceTabWidget->saveSettings();
    }
    // --- Network (teraz w AdvancedSettingsWidget) ---
    if (m_advancedTabWidget) { // <<< Dodano zapisywanie nowej zakładki
        m_advancedTabWidget->saveSettings();
    }

    if (m_shortcutsTabWidget) m_shortcutsTabWidget->saveSettings();

    // Zapisanie wszystkich ustawień do pliku/rejestru
    m_config->SaveSettings();

    ShortcutManager::GetInstance()->updateRegisteredShortcuts();

    QMessageBox::information(this, "Settings Saved", "Settings have been successfully saved.");
    emit settingsChanged(); // Emituj sygnał po zapisaniu
}

void SettingsView::restoreDefaults() {
    if (QMessageBox::question(this, "Restore Defaults",
                             "Are you sure you want to restore all settings to default values?\nThis includes appearance colors and network settings.", // Zaktualizowano komunikat
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_config->RestoreDefaults();
        loadSettingsFromRegistry(); // Odśwież cały widok, w tym delegowane ładowanie
        QMessageBox::information(this, "Defaults Restored", "Settings have been restored to their default values.");
        emit settingsChanged(); // Emituj sygnał również po przywróceniu domyślnych
                             }
}

void SettingsView::switchToTab(const int tabIndex) {
    if (tabIndex < 0 || tabIndex >= m_tabButtons.size()) return;

    for (int i = 0; i < m_tabButtons.size(); ++i) {
        m_tabButtons[i]->setActive(i == tabIndex);
        m_tabButtons[i]->setChecked(i == tabIndex);
    }
    m_tabContent->setCurrentIndex(tabIndex);

    constexpr int ClassifiedTabIndex = 4;
    if (tabIndex == ClassifiedTabIndex) {
        resetSecurityLayers();
    }
}

void SettingsView::handleBackButton() {
    resetSecurityLayers(); // Zresetuj warstwy przed wyjściem
    emit backToMainView(); // Wyemituj sygnał powrotu
}


void SettingsView::resetSecurityLayers() {
    // Implementacja bez zmian...
    if (!m_securityLayersStack || !m_fingerprintLayer || !m_handprintLayer ||
        !m_securityCodeLayer || !m_securityQuestionLayer || !m_retinaScanLayer ||
        !m_voiceRecognitionLayer || !m_typingTestLayer || !m_snakeGameLayer ||
        !m_classifiedFeaturesWidget) {
        qWarning() << "Cannot reset security layers: Stack or layers not fully initialized.";
        return;
    }

    qDebug() << "Resetting security layers... Debug mode:" << m_debugModeEnabled;

    m_fingerprintLayer->reset();
    m_handprintLayer->reset();
    m_securityCodeLayer->reset();
    m_securityQuestionLayer->reset();
    m_retinaScanLayer->reset();
    m_voiceRecognitionLayer->reset();
    m_typingTestLayer->reset();
    m_snakeGameLayer->reset();

    if (m_overrideButton) {
        m_overrideButton->setEnabled(true);
        m_overrideButton->setText("CAUTION: SYSTEM OVERRIDE");
    }

    if (m_debugModeEnabled) {
        const int featuresIndex = m_securityLayersStack->indexOf(m_classifiedFeaturesWidget);
        if (featuresIndex != -1) {
            m_securityLayersStack->setCurrentIndex(featuresIndex);
            qDebug() << "Debug mode: Bypassed security layers, showing classified features.";
        } else {
            qWarning() << "Debug mode error: Could not find ClassifiedFeaturesWidget index!";
            m_securityLayersStack->setCurrentIndex(FingerprintIndex);
            m_currentLayerIndex = FingerprintIndex;
            m_fingerprintLayer->initialize();
        }
    } else {
        m_currentLayerIndex = FingerprintIndex;
        m_securityLayersStack->setCurrentIndex(FingerprintIndex);
        m_fingerprintLayer->initialize();
        qDebug() << "Normal mode: Starting security layers sequence.";
    }
    qDebug() << "Security layers reset complete. Current index:" << m_securityLayersStack->currentIndex();
}