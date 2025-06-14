#include "settings_view.h"

#include <QHBoxLayout>
#include <QFormLayout>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QRandomGenerator>
#include <QTimer>

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
#include "../../ui/settings/tabs/shortcuts/shortcuts_settings_widget.h"

#include "../../app/managers/shortcut_manager.h"
#include "../../app/managers/translation_manager.h"
#include "../../app/wavelength_config.h"
#include "../buttons/tab_button.h"
#include "../buttons/cyber_button.h"

SettingsView::SettingsView(QWidget *parent)
    : QWidget(parent),
      config_(WavelengthConfig::GetInstance()),
      tab_content_(nullptr),
      title_label_(nullptr),
      session_label_(nullptr),
      time_label_(nullptr),
      tab_bar_(nullptr),
      wavelength_tab_widget_(nullptr),
      appearance_tab_widget_(nullptr),
      advanced_tab_widget_(nullptr),
      save_button_(nullptr),
      defaults_button_(nullptr),
      back_button_(nullptr),
      refresh_timer_(nullptr),
      fingerprint_layer_(nullptr),
      handprint_layer_(nullptr),
      security_code_layer_(nullptr),
      security_question_layer_(nullptr),
      retina_scan_layer_(nullptr),
      voice_recognition_layer_(nullptr),
      typing_test_layer_(nullptr),
      snake_game_layer_(nullptr),
      access_granted_widget_(nullptr),
      security_layers_stack_(nullptr),
      current_layer_index_(FingerprintIndex),
      debug_mode_enabled_(false),
      classified_features_widget_(nullptr),
      override_button_(nullptr),
      system_override_manager_(nullptr) {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("SettingsView { background-color: #101820; }");

    translator_ = TranslationManager::GetInstance();

    SetupUi();

    refresh_timer_ = new QTimer(this);
    refresh_timer_->setInterval(1000);
    connect(refresh_timer_, &QTimer::timeout, this, [this] {
        const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        if (time_label_) time_label_->setText(QString("TS: %1").arg(timestamp));
    });
    refresh_timer_->start();

    system_override_manager_ = new SystemOverrideManager(this);

    connect(system_override_manager_, &SystemOverrideManager::overrideFinished, this, [this] {
        qDebug() << "[SETTINGS VIEW] Override sequence finished signal received in SettingsView.";
        if (override_button_) {
            override_button_->setEnabled(true);
            override_button_->
                    setText(translator_->Translate("SettingsView.SystemOverride", "CAUTION: SYSTEM OVERRIDE"));
        }
    });
}

void SettingsView::SetDebugMode(const bool enabled) {
    if (debug_mode_enabled_ != enabled) {
        debug_mode_enabled_ = enabled;
        ResetSecurityLayers();
    }
}

SettingsView::~SettingsView() {
    if (refresh_timer_) {
        refresh_timer_->stop();
    }
}

void SettingsView::SetupUi() {
    const auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(40, 30, 40, 30);
    main_layout->setSpacing(15);

    CreateHeaderPanel();

    tab_bar_ = new QWidget(this);
    const auto tab_layout = new QHBoxLayout(tab_bar_);
    tab_layout->setContentsMargins(0, 5, 0, 10);
    tab_layout->setSpacing(0);
    tab_layout->addStretch(1);

    QStringList tab_names = {
        translator_->Translate("SettingsView.WavelengthTab", "Wavelength"),
        translator_->Translate("SettingsView.AppearanceTab", "Appearance"),
        translator_->Translate("SettingsView.NetworkTab", "Network"),
        translator_->Translate("SettingsView.ShortcutsTab", "Shortcuts"),
        translator_->Translate("SettingsView.ClassifiedTab", "CLASSIFIED"),
    };

    for (int i = 0; i < tab_names.size(); i++) {
        auto btn = new TabButton(tab_names[i], tab_bar_);
        if (tab_names[i] == "CLASSIFIED") {
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

        connect(btn, &QPushButton::clicked, this, [this, i] { SwitchToTab(i); });
        tab_layout->addWidget(btn);
        tab_buttons_.append(btn);
    }

    tab_layout->addStretch(1);
    main_layout->addWidget(tab_bar_);

    tab_content_ = new QStackedWidget(this);
    tab_content_->setStyleSheet(
        "QWidget { background-color: rgba(10, 25, 40, 180); border: 1px solid #005577; border-radius: 5px; }"
    );

    wavelength_tab_widget_ = new WavelengthSettingsWidget(tab_content_);
    tab_content_->addWidget(wavelength_tab_widget_); // index 0

    appearance_tab_widget_ = new AppearanceSettingsWidget(tab_content_);
    tab_content_->addWidget(appearance_tab_widget_); // index 1

    advanced_tab_widget_ = new NetworkSettingsWidget(tab_content_);
    tab_content_->addWidget(advanced_tab_widget_); // index 2

    shortcuts_tab_widget_ = new ShortcutsSettingsWidget(tab_content_);
    tab_content_->addWidget(shortcuts_tab_widget_); // index 3

    SetupClassifiedTab(); // index 4

    main_layout->addWidget(tab_content_, 1);

    const auto button_layout = new QHBoxLayout();
    button_layout->setSpacing(15);

    back_button_ = new CyberButton(
        translator_->Translate("SettingsView.BackButton", "BACK"),
        this, false);
    back_button_->setFixedHeight(40);

    defaults_button_ = new CyberButton(
        translator_->Translate("SettingsView.RestoreButton", "RESTORE DEFAULTS"),
        this, false);
    defaults_button_->setFixedHeight(40);

    save_button_ = new CyberButton(
        translator_->Translate("SettingsView.SaveButton", "SAVE SETTINGS"),
        this, true);
    save_button_->setFixedHeight(40);

    button_layout->addWidget(back_button_, 1);
    button_layout->addStretch(1);
    button_layout->addWidget(defaults_button_, 1);
    button_layout->addWidget(save_button_, 2);
    main_layout->addLayout(button_layout);

    connect(back_button_, &CyberButton::clicked, this, &SettingsView::HandleBackButton);
    connect(save_button_, &CyberButton::clicked, this, &SettingsView::SaveSettings);
    connect(defaults_button_, &CyberButton::clicked, this, &SettingsView::RestoreDefaults);

    if (!tab_buttons_.isEmpty()) {
        tab_buttons_[0]->SetActive(true);
        tab_buttons_[0]->setChecked(true);
    }
    tab_content_->setCurrentIndex(0);
}

void SettingsView::SetupClassifiedTab() {
    const auto tab = new QWidget(tab_content_);
    const auto layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    const auto info_label = new QLabel(
        translator_->Translate("SettingsView.ClassifiedInfo",
                               "CLASSIFIED INFORMATION - SECURITY VERIFICATION REQUIRED"),
        tab);
    info_label->setStyleSheet(
        "color: #ff3333; background-color: transparent; font-family: Consolas; font-size: 12pt; font-weight: bold;");
    layout->addWidget(info_label);

    const auto warning_label = new QLabel(
        translator_->Translate("SettingsView.ClassifiedSubtitle",
                               "Authorization required to access classified settings.\nMultiple security layers will be enforced."),
        tab);
    warning_label->setStyleSheet(
        "color: #ffcc00; background-color: transparent; font-family: Consolas; font-size: 9pt;");
    layout->addWidget(warning_label);

    security_layers_stack_ = new QStackedWidget(tab);
    layout->addWidget(security_layers_stack_, 1);

    fingerprint_layer_ = new FingerprintLayer(security_layers_stack_);
    handprint_layer_ = new HandprintLayer(security_layers_stack_);
    security_code_layer_ = new SecurityCodeLayer(security_layers_stack_);
    security_question_layer_ = new SecurityQuestionLayer(security_layers_stack_);
    retina_scan_layer_ = new RetinaScanLayer(security_layers_stack_);
    voice_recognition_layer_ = new VoiceRecognitionLayer(security_layers_stack_);
    typing_test_layer_ = new TypingTestLayer(security_layers_stack_);
    snake_game_layer_ = new SnakeGameLayer(security_layers_stack_);

    classified_features_widget_ = new QWidget(security_layers_stack_);
    const auto features_layout = new QVBoxLayout(classified_features_widget_);
    features_layout->setAlignment(Qt::AlignCenter);
    features_layout->setSpacing(20);

    const auto features_title = new QLabel(
        translator_->Translate("SettingsView.ClassifiedUnlocked", "CLASSIFIED FEATURES UNLOCKED"),
        classified_features_widget_);
    features_title->setStyleSheet("color: #33ff33; font-family: Consolas; font-size: 14pt; font-weight: bold;");
    features_title->setAlignment(Qt::AlignCenter);
    features_layout->addWidget(features_title);

    const auto features_desc = new QLabel(
        translator_->Translate("SettingsView.ClassifiedUnlockedSubtitle",
                               "You have bypassed all security layers.\nThe true potential is now available."),
        classified_features_widget_);
    features_desc->setStyleSheet("color: #cccccc; font-family: Consolas; font-size: 10pt;");
    features_desc->setAlignment(Qt::AlignCenter);
    features_layout->addWidget(features_desc);

    const auto button_container = new QWidget(classified_features_widget_);
    const auto button_layout_classified = new QHBoxLayout(button_container); // Zmieniono nazwę zmiennej
    button_layout_classified->setSpacing(15);

    override_button_ = new QPushButton(
        translator_->Translate("SettingsView.SystemOverride", "CAUTION: SYSTEM OVERRIDE"),
        button_container);
    override_button_->setMinimumHeight(50);
    override_button_->setMinimumWidth(250);
    override_button_->setStyleSheet(
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
    override_button_->setEnabled(true);
    connect(override_button_, &QPushButton::clicked, this, [this] {
        qDebug() << "[SETTINGS VIEW] Override button clicked.";
#ifdef Q_OS_WIN
        if (SystemOverrideManager::IsRunningAsAdmin()) {
            qDebug() << "[SETTINGS VIEW] Already running as admin. Initiating sequence directly.";
            override_button_->setEnabled(false);
            override_button_->setText("OVERRIDE IN PROGRESS...");
            QMetaObject::invokeMethod(system_override_manager_, "InitiateOverrideSequence", Qt::QueuedConnection, Q_ARG(bool, false));
        } else {
            qDebug() << "[SETTINGS VIEW] Not running as admin. Attempting relaunch with elevation request...";
            QStringList args;
            args << "--run-override";
            if (SystemOverrideManager::RelaunchAsAdmin(args)) {
                qDebug() << "[SETTINGS VIEW] Relaunch initiated. Closing current instance.";
                QApplication::quit();
            } else {
                QMessageBox::warning(this, "Elevation Failed", "Could not request administrator privileges.\nThe override sequence cannot start.");
            }
        }
#else
        QMessageBox::information(this,
                                 translator_->Translate("SettingsView.SystemOverride", "System Override"),
                                 translator_->Translate("SettingsView.SystemOverrideInitiated",
                                                        "System override sequence initiated (OS specific elevation might be required).")
        );
        override_button_->setEnabled(false);
        override_button_->setText(
            translator_->Translate("SettingsView.SystemOverrideInProgress", "OVERRIDE IN PROGRESS..."));
        QMetaObject::invokeMethod(system_override_manager_, "InitiateOverrideSequence", Qt::QueuedConnection,
                                  Q_ARG(bool, false));
#endif
    });
    button_layout_classified->addWidget(override_button_);

    features_layout->addWidget(button_container, 0, Qt::AlignCenter);
    features_layout->addStretch();

    security_layers_stack_->addWidget(fingerprint_layer_);
    security_layers_stack_->addWidget(handprint_layer_);
    security_layers_stack_->addWidget(security_code_layer_);
    security_layers_stack_->addWidget(security_question_layer_);
    security_layers_stack_->addWidget(retina_scan_layer_);
    security_layers_stack_->addWidget(voice_recognition_layer_);
    security_layers_stack_->addWidget(typing_test_layer_);
    security_layers_stack_->addWidget(snake_game_layer_);
    security_layers_stack_->addWidget(classified_features_widget_);

    connect(fingerprint_layer_, &SecurityLayer::layerCompleted, this, [this] {
        current_layer_index_ = HandprintIndex;
        security_layers_stack_->setCurrentIndex(current_layer_index_);
        handprint_layer_->Initialize();
    });
    connect(handprint_layer_, &SecurityLayer::layerCompleted, this, [this] {
        current_layer_index_ = SecurityCodeIndex;
        security_layers_stack_->setCurrentIndex(current_layer_index_);
        security_code_layer_->Initialize();
    });
    connect(security_code_layer_, &SecurityLayer::layerCompleted, this, [this] {
        current_layer_index_ = SecurityQuestionIndex;
        security_layers_stack_->setCurrentIndex(current_layer_index_);
        security_question_layer_->Initialize();
    });
    connect(security_question_layer_, &SecurityLayer::layerCompleted, this, [this] {
        current_layer_index_ = RetinaScanIndex;
        security_layers_stack_->setCurrentIndex(current_layer_index_);
        retina_scan_layer_->Initialize();
    });
    connect(retina_scan_layer_, &SecurityLayer::layerCompleted, this, [this] {
        current_layer_index_ = VoiceRecognitionIndex;
        security_layers_stack_->setCurrentIndex(current_layer_index_);
        voice_recognition_layer_->Initialize();
    });
    connect(voice_recognition_layer_, &SecurityLayer::layerCompleted, this, [this] {
        current_layer_index_ = TypingTestIndex;
        security_layers_stack_->setCurrentIndex(current_layer_index_);
        typing_test_layer_->Initialize();
    });
    connect(typing_test_layer_, &SecurityLayer::layerCompleted, this, [this] {
        current_layer_index_ = SnakeGameIndex;
        security_layers_stack_->setCurrentIndex(current_layer_index_);
        snake_game_layer_->Initialize();
    });
    connect(snake_game_layer_, &SecurityLayer::layerCompleted, this, [this] {
        int features_index = security_layers_stack_->indexOf(classified_features_widget_);
        if (features_index != -1) {
            current_layer_index_ = static_cast<SecurityLayerIndex>(features_index);
            security_layers_stack_->setCurrentIndex(features_index);
        } else {
            qWarning() << "[SETTINGS VIEW] Could not find ClassifiedFeaturesWidget in the stack!";
            security_layers_stack_->setCurrentIndex(security_layers_stack_->count() - 1);
        }
    });

    layout->addStretch();
    tab_content_->addWidget(tab);

    ResetSecurityLayers();
}

void SettingsView::SetupNextSecurityLayer() {
    const int features_index = security_layers_stack_->indexOf(classified_features_widget_);
    if (features_index == -1) {
        qWarning() << "[SETTINGS VIEW] Cannot setup next layer: ClassifiedFeaturesWidget not found.";
        return;
    }

    if (static_cast<int>(current_layer_index_) < features_index) {
        current_layer_index_ = static_cast<SecurityLayerIndex>(static_cast<int>(current_layer_index_) + 1);
    } else return;

    if (current_layer_index_ < features_index) {
        security_layers_stack_->setCurrentIndex(current_layer_index_);
        QWidget *current_widget = security_layers_stack_->widget(current_layer_index_);
        if (const auto current_layer = qobject_cast<SecurityLayer *>(current_widget)) {
            current_layer->Initialize();
        } else {
            qWarning() << "[SETTINGS VIEW] Widget at index" << current_layer_index_ << "is not a SecurityLayer!";
        }
    } else if (current_layer_index_ == features_index) {
        security_layers_stack_->setCurrentIndex(features_index);
    }
}

void SettingsView::CreateHeaderPanel() {
    const auto header_layout = new QHBoxLayout();
    header_layout->setSpacing(10);

    title_label_ = new QLabel(
        translator_->Translate("SettingsView.SystemSettings", "SYSTEM SETTINGS"),
        this);
    title_label_->setStyleSheet(
        "color: #00ccff; background-color: transparent; font-family: Consolas; font-size: 15pt; letter-spacing: 2px;");

    header_layout->addWidget(title_label_);
    header_layout->addStretch();

    const QString session_id = QString("%1-%2")
            .arg(QRandomGenerator::global()->bounded(1000, 9999))
            .arg(QRandomGenerator::global()->bounded(10000, 99999));

    session_label_ = new QLabel(QString("SESSION_ID: %1").arg(session_id), this);
    session_label_->setStyleSheet(
        "color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

    header_layout->addWidget(session_label_);

    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    time_label_ = new QLabel(QString("TS: %1").arg(timestamp), this);
    time_label_->setStyleSheet("color: #00aa88; background-color: transparent; font-family: Consolas; font-size: 8pt;");

    header_layout->addWidget(time_label_);

    if (const auto main_v_layout = qobject_cast<QVBoxLayout *>(layout())) {
        main_v_layout->insertLayout(0, header_layout);
    } else {
        qWarning() << "[SETTINGS VIEW] Could not cast main layout to QVBoxLayout in createHeaderPanel";
        layout()->addItem(header_layout);
    }
}

void SettingsView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    LoadSettingsFromRegistry();
    ResetSecurityLayers();
}


void SettingsView::LoadSettingsFromRegistry() const {
    if (wavelength_tab_widget_) {
        wavelength_tab_widget_->LoadSettings();
    }
    if (appearance_tab_widget_) {
        appearance_tab_widget_->LoadSettings();
    }
    if (advanced_tab_widget_) {
        advanced_tab_widget_->LoadSettings();
    }

    if (shortcuts_tab_widget_) shortcuts_tab_widget_->LoadSettings();
}

void SettingsView::SaveSettings() {
    if (wavelength_tab_widget_) {
        wavelength_tab_widget_->SaveSettings();
    }
    if (appearance_tab_widget_) {
        appearance_tab_widget_->SaveSettings();
    }
    if (advanced_tab_widget_) {
        advanced_tab_widget_->SaveSettings();
    }

    if (shortcuts_tab_widget_) shortcuts_tab_widget_->SaveSettings();

    config_->SaveSettings();

    ShortcutManager::GetInstance()->updateRegisteredShortcuts();

    QMessageBox::information(this,
                             translator_->Translate("SettingsView.SettingsSavedTitle", "Settings Saved"),
                             translator_->Translate("SettingsView.SettingsSavedMessage",
                                                    "Settings have been successfully saved.")
    );
    emit settingsChanged();
}

void SettingsView::RestoreDefaults() {
    if (QMessageBox::question(this,
                              translator_->Translate("SettingsView.RestoreDefaultsTitle", "Restore Defaults"),
                              translator_->Translate("SettingsView.RestoreDefaultsMessage",
                                                     "Are you sure you want to restore all settings to default values?\nThis includes appearance colors and network settings."),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        config_->RestoreDefaults();
        LoadSettingsFromRegistry();
        QMessageBox::information(this,
                                 translator_->Translate("SettingsView.RestoreDefaultsSuccessTitle",
                                                        "Defaults Restored"),
                                 translator_->Translate("SettingsView.RestoreDefaultsSuccessMessage",
                                                        "Settings have been restored to their default values.")
        );
        emit settingsChanged();
    }
}

void SettingsView::SwitchToTab(const int tab_index) {
    if (tab_index < 0 || tab_index >= tab_buttons_.size()) return;

    for (int i = 0; i < tab_buttons_.size(); ++i) {
        tab_buttons_[i]->SetActive(i == tab_index);
        tab_buttons_[i]->setChecked(i == tab_index);
    }
    tab_content_->setCurrentIndex(tab_index);

    constexpr int classified_tab_index = 4;
    if (tab_index == classified_tab_index) {
        ResetSecurityLayers();
    }
}

void SettingsView::HandleBackButton() {
    ResetSecurityLayers();
    emit backToMainView();
}


void SettingsView::ResetSecurityLayers() {
    if (!security_layers_stack_ || !fingerprint_layer_ || !handprint_layer_ ||
        !security_code_layer_ || !security_question_layer_ || !retina_scan_layer_ ||
        !voice_recognition_layer_ || !typing_test_layer_ || !snake_game_layer_ ||
        !classified_features_widget_) {
        qWarning() << "[SETTINGS VIEW] Cannot reset security layers: Stack or layers not fully initialized.";
        return;
    }

    fingerprint_layer_->Reset();
    handprint_layer_->Reset();
    security_code_layer_->Reset();
    security_question_layer_->Reset();
    retina_scan_layer_->Reset();
    voice_recognition_layer_->Reset();
    typing_test_layer_->Reset();
    snake_game_layer_->Reset();

    if (override_button_) {
        override_button_->setEnabled(true);
        override_button_->setText(
            translator_->Translate("SettingsView.SystemOverride", "CAUTION: SYSTEM OVERRIDE")
        );
    }

    if (debug_mode_enabled_) {
        const int featuresIndex = security_layers_stack_->indexOf(classified_features_widget_);
        if (featuresIndex != -1) {
            security_layers_stack_->setCurrentIndex(featuresIndex);
        } else {
            qWarning() << "[SETTINGS VIEW] Debug mode error: Could not find ClassifiedFeaturesWidget index!";
            security_layers_stack_->setCurrentIndex(FingerprintIndex);
            current_layer_index_ = FingerprintIndex;
            fingerprint_layer_->Initialize();
        }
    } else {
        current_layer_index_ = FingerprintIndex;
        security_layers_stack_->setCurrentIndex(FingerprintIndex);
        fingerprint_layer_->Initialize();
    }
}
