#include "settings_view.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QStyle>

#include "settings/classified/system_override_manager.h"

SettingsView::SettingsView(QWidget *parent)
    : QWidget(parent),
m_config(WavelengthConfig::getInstance()),
m_securityLayersStack(nullptr),
m_fingerprintLayer(nullptr),
m_handprintLayer(nullptr),
m_securityCodeLayer(nullptr),
m_securityQuestionLayer(nullptr),
m_retinaScanLayer(nullptr),
m_voiceRecognitionLayer(nullptr),
m_typingTestLayer(nullptr),
m_snakeGameLayer(nullptr),
m_classifiedFeaturesWidget(nullptr),
m_waveSculptorWindow(nullptr),
m_currentLayerIndex(FingerprintIndex),
m_debugModeEnabled(true) // <<< Zainicjalizuj flagę na false
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("SettingsView { background-color: #101820; }");

    setupUi();

    // Timer do aktualizacji czasu
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        m_timeLabel->setText(QString("TS: %1").arg(timestamp));
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

void SettingsView::setDebugMode(bool enabled) {
    if (m_debugModeEnabled != enabled) {
        m_debugModeEnabled = enabled;
        qDebug() << "SettingsView Debug Mode:" << (enabled ? "ENABLED" : "DISABLED");
        // Po zmianie trybu debugowania, zresetuj stan zakładki CLASSIFIED
        resetSecurityLayers();
    }
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

    // Panel zakładek (przyciski zamiast QTabWidget)
    m_tabBar = new QWidget(this);
    QHBoxLayout *tabLayout = new QHBoxLayout(m_tabBar);
    tabLayout->setContentsMargins(0, 5, 0, 10);
    tabLayout->setSpacing(0);
    tabLayout->addStretch(1);

    // Tworzenie przycisków zakładek
    QStringList tabNames = {"User", "Server", "Appearance", "Network", "Advanced", "CLASSIFIED"};

    for (int i = 0; i < tabNames.size(); i++) {
        TabButton *btn = new TabButton(tabNames[i], m_tabBar);

        // Specjalne formatowanie dla zakładki CLASSIFIED
        if (tabNames[i] == "CLASSIFIED") {
            btn->setStyleSheet(
                "TabButton {"
                "  color: #ff3333;"  // czerwony kolor
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
    setupUserTab();
    setupServerTab();
    setupAppearanceTab();
    setupNetworkTab();
    setupAdvancedTab();
    setupClassifiedTab();

    mainLayout->addWidget(m_tabContent, 1);

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
    connect(m_backButton, &CyberButton::clicked, this, &SettingsView::handleBackButton);
    connect(m_saveButton, &CyberButton::clicked, this, &SettingsView::saveSettings);
    connect(m_defaultsButton, &CyberButton::clicked, this, &SettingsView::restoreDefaults);

    // Domyślnie aktywna pierwsza zakładka
    m_tabButtons[0]->setActive(true);
    m_tabButtons[0]->setChecked(true);
    m_tabContent->setCurrentIndex(0);
}

void SettingsView::setupClassifiedTab() {
    QWidget *tab = new QWidget(m_tabContent);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    QLabel *infoLabel = new QLabel("CLASSIFIED INFORMATION - SECURITY VERIFICATION REQUIRED", tab);
    infoLabel->setStyleSheet("color: #ff3333; background-color: transparent; font-family: Consolas; font-size: 12pt; font-weight: bold;");
    layout->addWidget(infoLabel);

    QLabel *warningLabel = new QLabel("Authorization required to access classified settings.\nMultiple security layers will be enforced.", tab);
    warningLabel->setStyleSheet("color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(warningLabel);

    // Kontener na wszystkie poziomy zabezpieczeń i nowe funkcje
    m_securityLayersStack = new QStackedWidget(tab);
    layout->addWidget(m_securityLayersStack, 1);

    // Tworzenie warstw zabezpieczeń (bez zmian)
    m_fingerprintLayer = new FingerprintLayer(m_securityLayersStack);
    m_handprintLayer = new HandprintLayer(m_securityLayersStack);
    m_securityCodeLayer = new SecurityCodeLayer(m_securityLayersStack);
    m_securityQuestionLayer = new SecurityQuestionLayer(m_securityLayersStack);
    m_retinaScanLayer = new RetinaScanLayer(m_securityLayersStack);
    m_voiceRecognitionLayer = new VoiceRecognitionLayer(m_securityLayersStack);
    m_typingTestLayer = new TypingTestLayer(m_securityLayersStack);
    m_snakeGameLayer = new SnakeGameLayer(m_securityLayersStack);

    // --- NOWY WIDGET Z FUNKCJAMI ZAMIAST ACCESS GRANTED ---
    m_classifiedFeaturesWidget = new QWidget(m_securityLayersStack);
    QVBoxLayout *featuresLayout = new QVBoxLayout(m_classifiedFeaturesWidget);
    featuresLayout->setAlignment(Qt::AlignCenter);
    featuresLayout->setSpacing(20);

    QLabel *featuresTitle = new QLabel("CLASSIFIED FEATURES UNLOCKED", m_classifiedFeaturesWidget);
    featuresTitle->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 14pt; font-weight: bold;");
    featuresTitle->setAlignment(Qt::AlignCenter);
    featuresLayout->addWidget(featuresTitle);

    QLabel *featuresDesc = new QLabel("You have bypassed all security layers.\nThe true potential is now available.", m_classifiedFeaturesWidget);
    featuresDesc->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    featuresDesc->setAlignment(Qt::AlignCenter);
    featuresLayout->addWidget(featuresDesc);

    // Kontener na przyciski funkcji
    QWidget* buttonContainer = new QWidget(m_classifiedFeaturesWidget);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setSpacing(15);

    // Przycisk Wave Sculptor
    QPushButton* waveSculptorButton = new QPushButton("Wave Sculptor", buttonContainer);
    waveSculptorButton->setMinimumHeight(40);
    waveSculptorButton->setStyleSheet(
        "QPushButton { background-color: #0077AA; border: 1px solid #00AAFF; padding: 10px; border-radius: 5px; color: #E0E0E0; font-family: Consolas; font-weight: bold; }"
        "QPushButton:hover { background-color: #0099CC; }"
        "QPushButton:pressed { background-color: #005588; }"
    );
    connect(waveSculptorButton, &QPushButton::clicked, this, &SettingsView::openWaveSculptor);
    buttonLayout->addWidget(waveSculptorButton);

    // Miejsca na przyciski Matrix Vision i System Override (dodamy później)
    QPushButton* matrixVisionButton = new QPushButton("Matrix Vision (TBD)", buttonContainer);
    matrixVisionButton->setMinimumHeight(40);
    matrixVisionButton->setStyleSheet("QPushButton { background-color: #444; border: 1px solid #666; padding: 10px; border-radius: 5px; color: #999; }");
    matrixVisionButton->setEnabled(false); // Na razie wyłączony
    buttonLayout->addWidget(matrixVisionButton);

    // Przycisk SYSTEM OVERRIDE - Poprawiona inicjalizacja
    m_overrideButton = new QPushButton("CAUTION: SYSTEM OVERRIDE", buttonContainer); // Poprawiony tekst
    m_overrideButton->setMinimumHeight(50); // Wyższy przycisk
    m_overrideButton->setMinimumWidth(250);
    m_overrideButton->setStyleSheet( // Zastosuj od razu właściwy styl
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
        "QPushButton:disabled {" // Dodaj styl dla stanu wyłączonego
        "  background-color: #555;"
        "  border-color: #777;"
        "  color: #999;"
        "}"
    );
    m_overrideButton->setEnabled(true); // Upewnij się, że jest włączony
    // --- DODAJ TO POŁĄCZENIE ---
    connect(m_overrideButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Override button clicked.";

#ifdef Q_OS_WIN
        if (SystemOverrideManager::isRunningAsAdmin()) {
            // Już mamy uprawnienia, kontynuuj normalnie
            qDebug() << "Already running as admin. Initiating sequence directly.";
            m_overrideButton->setEnabled(false);
            m_overrideButton->setText("OVERRIDE IN PROGRESS...");
            // Wywołaj sekwencję (przekaż odpowiednią wartość dla isFirstTime)
            QMetaObject::invokeMethod(m_systemOverrideManager, "initiateOverrideSequence", Qt::QueuedConnection, Q_ARG(bool, false)); // Użyj invokeMethod dla bezpieczeństwa wątków, jeśli jest ryzyko
        } else {
            // Nie mamy uprawnień, poproś o nie przez ponowne uruchomienie
            qDebug() << "Not running as admin. Attempting relaunch with elevation request...";
            // Dodaj argument, aby nowa instancja wiedziała, co robić
            QStringList args;
            args << "--run-override"; // Specjalny argument

            if (SystemOverrideManager::relaunchAsAdmin(args)) {
                // Próba ponownego uruchomienia zainicjowana (UAC powinno się pojawić)
                // Zamknij bieżącą instancję aplikacji, aby uniknąć duplikatów
                qDebug() << "Relaunch initiated. Closing current instance.";
                QApplication::quit();
            } else {
                // Nie udało się nawet poprosić o uprawnienia (np. błąd systemowy)
                // lub użytkownik anulował UAC (choć to trudne do wykrycia tutaj od razu)
                QMessageBox::warning(this, "Elevation Failed", "Could not request administrator privileges.\nThe override sequence cannot start.");
                // Przycisk pozostaje włączony
            }
        }
#else
        // Obsługa dla innych systemów (np. Linux/macOS używające sudo/pkexec)
        QMessageBox::information(this, "System Override", "System override sequence initiated (OS specific elevation might be required).");
        m_overrideButton->setEnabled(false);
        m_overrideButton->setText("OVERRIDE IN PROGRESS...");
        QMetaObject::invokeMethod(m_systemOverrideManager, "initiateOverrideSequence", Qt::QueuedConnection, Q_ARG(bool, false));
#endif
    });
    // ---------------------------
    buttonLayout->addWidget(m_overrideButton);


    featuresLayout->addWidget(buttonContainer, 0, Qt::AlignCenter);
    featuresLayout->addStretch();
    // --- KONIEC NOWEGO WIDGETU ---

    // Dodajemy wszystkie widgety do stosu
    m_securityLayersStack->addWidget(m_fingerprintLayer);
    m_securityLayersStack->addWidget(m_handprintLayer);
    m_securityLayersStack->addWidget(m_securityCodeLayer);
    m_securityLayersStack->addWidget(m_securityQuestionLayer);
    m_securityLayersStack->addWidget(m_retinaScanLayer);
    m_securityLayersStack->addWidget(m_voiceRecognitionLayer);
    m_securityLayersStack->addWidget(m_typingTestLayer);
    m_securityLayersStack->addWidget(m_snakeGameLayer);
    m_securityLayersStack->addWidget(m_classifiedFeaturesWidget);

    // Połączenia warstw
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
        m_currentLayerIndex = RetinaScanIndex; // Zmiana na RetinaScanIndex
        m_securityLayersStack->setCurrentIndex(m_currentLayerIndex);
        m_retinaScanLayer->initialize(); // Inicjalizacja nowej warstwy
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
        // Zamiast AccessGrantedIndex, użyjemy indeksu nowego widgetu
        int featuresIndex = m_securityLayersStack->indexOf(m_classifiedFeaturesWidget);
        if (featuresIndex != -1) {
             m_currentLayerIndex = static_cast<SecurityLayerIndex>(featuresIndex); // Rzutowanie może być ryzykowne, jeśli indeksy się zmienią
             m_securityLayersStack->setCurrentIndex(featuresIndex);
             // Nie ma potrzeby initialize() dla tego widgetu
             qDebug() << "All security layers passed. Showing classified features.";
        } else {
             qWarning() << "Could not find ClassifiedFeaturesWidget in the stack!";
             // Awaryjnie, przejdź do ostatniego widgetu
             m_securityLayersStack->setCurrentIndex(m_securityLayersStack->count() - 1);
        }
    });

    layout->addStretch();
    m_tabContent->addWidget(tab);

    resetSecurityLayers();
}

void SettingsView::openWaveSculptor() {
    qDebug() << "Opening Wave Sculptor...";
    // Tworzymy nowe okno za każdym razem lub pokazujemy istniejące
    // Prostsze podejście: tworzymy nowe za każdym razem
    // Pamiętaj, że WaveSculptorWindow usuwa się samo po zamknięciu (deleteLater)
    WaveSculptorWindow* sculptorWindow = new WaveSculptorWindow();
    sculptorWindow->show();
}

void SettingsView::setupNextSecurityLayer() {
    // Sprawdź, czy obecny indeks jest prawidłowy i czy nie jest to ostatnia warstwa przed dostępem
    if (m_currentLayerIndex < AccessGrantedIndex) {
        m_currentLayerIndex = static_cast<SecurityLayerIndex>(static_cast<int>(m_currentLayerIndex) + 1);
    } else {
        // Jeśli już jesteśmy na AccessGrantedIndex lub dalej, nie rób nic
        return;
    }


    // Przygotowanie następnego zabezpieczenia
    switch (m_currentLayerIndex) {
        case HandprintIndex:
            m_securityLayersStack->setCurrentIndex(HandprintIndex);
        m_handprintLayer->initialize();
        break;
        case SecurityCodeIndex:
            m_securityLayersStack->setCurrentIndex(SecurityCodeIndex);
        m_securityCodeLayer->initialize();
        break;
        case SecurityQuestionIndex:
            m_securityLayersStack->setCurrentIndex(SecurityQuestionIndex);
        m_securityQuestionLayer->initialize();
        break;
        case RetinaScanIndex:
            m_securityLayersStack->setCurrentIndex(RetinaScanIndex);
        m_retinaScanLayer->initialize();
        break;
        case VoiceRecognitionIndex:
            m_securityLayersStack->setCurrentIndex(VoiceRecognitionIndex);
        m_voiceRecognitionLayer->initialize();
        break;
        case TypingTestIndex:
            m_securityLayersStack->setCurrentIndex(TypingTestIndex);
        m_typingTestLayer->initialize();
        break;
        case SnakeGameIndex:
            m_securityLayersStack->setCurrentIndex(SnakeGameIndex);
        m_snakeGameLayer->initialize();
        break;
        case AccessGrantedIndex:
            m_securityLayersStack->setCurrentIndex(AccessGrantedIndex);
        // Tutaj nie ma initialize(), bo to ekran końcowy
        break;
        // Nie powinno być default, bo wszystkie przypadki są obsłużone
    }
}

void SettingsView::setupUserTab() {
    QWidget *tab = new QWidget(m_tabContent);
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

    m_tabContent->addWidget(tab);
}

void SettingsView::setupServerTab() {
    QWidget *tab = new QWidget(m_tabContent);
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

    m_tabContent->addWidget(tab);
}

void SettingsView::setupAppearanceTab() {
    QWidget *tab = new QWidget(m_tabContent);
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

    layout->addLayout(formLayout);
    layout->addStretch();

    m_tabContent->addWidget(tab);
}

void SettingsView::setupNetworkTab() {
    QWidget *tab = new QWidget(m_tabContent);
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

    m_tabContent->addWidget(tab);
}

void SettingsView::setupAdvancedTab() {
    QWidget *tab = new QWidget(m_tabContent);
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

    m_tabContent->addWidget(tab);
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
    resetSecurityLayers();
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
        if (m_tabContent->currentIndex() == 5) {
            resetSecurityLayers();
        }
                             }
}

void SettingsView::switchToTab(int tabIndex) {
    // Sprawdź, czy próbujemy przełączyć się do tej samej zakładki
    if (tabIndex == m_tabContent->currentIndex()) {
        return; // Nie rób nic, jeśli to ta sama zakładka
    }

    // Ustaw aktywny przycisk zakładki
    for (int i = 0; i < m_tabButtons.size(); i++) {
        m_tabButtons[i]->setActive(i == tabIndex);
        m_tabButtons[i]->setChecked(i == tabIndex);
    }

    // Przełącz zawartość zakładki
    m_tabContent->setCurrentIndex(tabIndex);

    // Usunięto blok resetujący warstwy przy każdym przejściu do CLASSIFIED
    // Resetowanie odbywa się teraz tylko przy wejściu do widoku (showEvent)
    // lub przez przycisk BACK (handleBackButton) lub przy restoreDefaults.
    // Jeśli zakładka CLASSIFIED nie została jeszcze zainicjowana (np. pierwszy raz),
    // resetSecurityLayers() w setupClassifiedTab() ustawi ją poprawnie.
    // Jeśli użytkownik przeszedł warstwy, stan zostanie zachowany do opuszczenia widoku.
}

void SettingsView::handleBackButton() {
    resetSecurityLayers(); // Zresetuj warstwy przed wyjściem
    emit backToMainView(); // Wyemituj sygnał powrotu
}


void SettingsView::resetSecurityLayers() {
    // Sprawdź, czy widgety warstw i stos zostały utworzone
    if (!m_securityLayersStack || !m_fingerprintLayer || !m_handprintLayer ||
        !m_securityCodeLayer || !m_securityQuestionLayer || !m_retinaScanLayer ||
        !m_voiceRecognitionLayer || !m_typingTestLayer || !m_snakeGameLayer ||
        !m_classifiedFeaturesWidget) {
        qWarning() << "Cannot reset security layers: Stack or layers not fully initialized.";
        return;
    }

    qDebug() << "Resetting security layers... Debug mode:" << m_debugModeEnabled;

    // Zresetuj stan każdej warstwy (zawsze warto to zrobić)
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
        // --- TRYB DEBUGOWANIA ---
        // Znajdź indeks ostatniego widgetu (funkcji)
        int featuresIndex = m_securityLayersStack->indexOf(m_classifiedFeaturesWidget);
        if (featuresIndex != -1) {
            m_securityLayersStack->setCurrentIndex(featuresIndex);
            qDebug() << "Debug mode: Bypassed security layers, showing classified features.";
            // Nie ma potrzeby wywoływania initialize() dla tego widgetu
        } else {
            qWarning() << "Debug mode error: Could not find ClassifiedFeaturesWidget index!";
            // Awaryjnie, ustaw pierwszy widget
            m_securityLayersStack->setCurrentIndex(FingerprintIndex);
            m_currentLayerIndex = FingerprintIndex;
            m_fingerprintLayer->initialize();
        }
    } else {
        // --- TRYB NORMALNY ---
        // Ustaw pierwszą warstwę jako aktywną i zainicjuj ją
        m_currentLayerIndex = FingerprintIndex;
        m_securityLayersStack->setCurrentIndex(FingerprintIndex);
        m_fingerprintLayer->initialize();
        qDebug() << "Normal mode: Starting security layers sequence.";
    }
    qDebug() << "Security layers reset complete. Current index:" << m_securityLayersStack->currentIndex();
}

