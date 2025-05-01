#include <QOperatingSystemVersion>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

#include <QMainWindow>
#include "src/ui/navigation/navbar.h"
#include "src/blob/core/blob_animation.h"
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QStyleFactory>

#include "src/app/managers/font_manager.h"
#include "src/app/managers/app_instance_manager.h"
#include "src/ui/dialogs/join_wavelength_dialog.h"
#include "src/ui/views/wavelength_chat_view.h"
#include "src/ui/dialogs/wavelength_dialog.h"

#include "src/session/wavelength_session_coordinator.h"
#include "src/app/style/cyberpunk_style.h"
#include "src/ui/effects/cyberpunk_text_effect.h"
#include "src/ui/widgets/animated_stacked_widget.h"
#include "src/app/managers/shortcut_manager.h"
#include "src/ui/views/settings_view.h"
#include "src/ui/settings/tabs/classified/components/system_override_manager.h"

void centerLabel(QLabel *label, const BlobAnimation *animation) {
    if (label && animation) {
        label->setGeometry(
            (animation->width() - label->sizeHint().width()) / 2,
            (animation->height() - label->sizeHint().height()) / 2,
            label->sizeHint().width(),
            label->sizeHint().height()
        );
        label->raise();
    }
}

void updateTitleLabelStyle(QLabel* label, const QColor& textColor, const QColor& borderColor) {
    if (!label) return;
    // Ostrożnie zrekonstruuj styl, zachowując inne właściwości
    const QString style = QString(
        "QLabel {"
        "   font-family: 'Blender Pro Heavy';" // Zachowaj czcionkę
        "   letter-spacing: 8px;"             // Zachowaj odstępy
        "   color: %1;"                       // NOWY kolor tekstu
        "   background-color: transparent;"   // Zachowaj tło
        "   border: 2px solid %2;"            // NOWY kolor ramki
        "   border-radius: 8px;"              // Zachowaj promień
        "   padding: 10px 20px;"              // Zachowaj padding
        "   text-transform: uppercase;"       // Zachowaj transformację
        "}"
    ).arg(textColor.name(QColor::HexRgb), borderColor.name(QColor::HexRgb));
    label->setStyleSheet(style);
    qDebug() << "Updated titleLabel style:" << style;
}

class ResizeEventFilter final : public QObject {
public:
    ResizeEventFilter(QLabel *label, BlobAnimation *animation)
        : QObject(animation), m_label(label), m_animation(animation) {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == m_animation && event->type() == QEvent::Resize) {
            centerLabel(m_label, m_animation);
            return false;
        }
        return false;
    }

private:
    QLabel *m_label;
    BlobAnimation *m_animation;
};


int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Wavelength");
    QCoreApplication::setApplicationName("WavelengthApp");

    auto bootSound = new QSoundEffect(&app);
    bootSound->setSource(QUrl("qrc:/resources/sounds/interface/boot_up.wav"));
    bootSound->setVolume(1.0); // Ustaw głośność (0.0 - 1.0)

    auto shutdownSound = new QSoundEffect(&app);
    shutdownSound->setSource(QUrl("qrc:/resources/sounds/interface/shutdown.wav"));
    shutdownSound->setVolume(1.0);



    WavelengthConfig *config = WavelengthConfig::GetInstance();

    // --- Przetwarzanie argumentów linii poleceń ---
    QCommandLineParser parser;
    parser.setApplicationDescription("Wavelength Application");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption overrideOption("run-override", "Internal flag to start the system override sequence immediately.");
    parser.addOption(overrideOption);
    parser.process(app);
    // ---------------------------------------------

    // --- Sprawdź, czy uruchomić sekwencję override ---
    if (parser.isSet(overrideOption)) {
        qDebug() << "--run-override flag detected.";
#ifdef Q_OS_WIN
        if (!SystemOverrideManager::IsRunningAsAdmin()) {
            qWarning() << "Override sequence requires administrator privileges. Attempting relaunch...";
            if (SystemOverrideManager::RelaunchAsAdmin(app.arguments())) {
                qDebug() << "Relaunch successful. Exiting current instance.";
                return 0; // Zakończ bieżącą instancję
            } else {
                qCritical() << "Failed to relaunch as administrator. Override aborted.";
                QMessageBox::critical(nullptr, "Admin Privileges Required", "Failed to relaunch with administrator privileges. The override sequence cannot continue.");
                return 1; // Zakończ z błędem
            }
        } else {
            qDebug() << "Running with administrator privileges.";
            SystemOverrideManager overrideManager; // Utwórz na stosie
            // Połącz sygnał zakończenia override z wyjściem z aplikacji
            QObject::connect(&overrideManager, &SystemOverrideManager::overrideFinished, &app, &QCoreApplication::quit);
            overrideManager.InitiateOverrideSequence(false); // Rozpocznij sekwencję
            return QApplication::exec(); // Uruchom pętlę zdarzeń TYLKO dla override
        }
#else
        qWarning() << "--run-override flag ignored on non-Windows OS.";
        // Kontynuuj normalne uruchomienie poniżej
#endif
    }

    if (bootSound->isLoaded()) {
        bootSound->play();
    } else {
        QTimer::singleShot(100, bootSound, [bootSound](){
            if(bootSound->isLoaded()) {
                bootSound->play();
            } else {
                 qWarning() << "Failed to load boot sound after delay:" << bootSound->source();
            }
        });
        qWarning() << "Boot sound not loaded immediately, attempting delayed play:" << bootSound->source();
    }
    // --- Koniec odtwarzania dźwięku startowego ---

    QObject::connect(&app, &QApplication::aboutToQuit, [shutdownSound]() {
    qDebug() << "DEBUG: Entering aboutToQuit lambda."; // <<< Log 1
    if (shutdownSound->isLoaded()) {
        qDebug() << "DEBUG: Shutdown sound is loaded. Attempting to play."; // <<< Log 2
        shutdownSound->play();
        // Dajmy BARDZO dużo czasu na próbę
        QEventLoop loop;
        QTimer::singleShot(1500, &loop, &QEventLoop::quit); // Czekaj 1 sekundę
        loop.exec();
        qDebug() << "DEBUG: Delay finished after playing sound."; // <<< Log 3

    } else {
        qWarning() << "Shutdown sound not loaded when aboutToQuit emitted:" << shutdownSound->source();
    }
});

    FontManager* fontManager = FontManager::GetInstance();
    if (!fontManager->Initialize()) {
        qWarning() << "Uwaga: Nie wszystkie czcionki zostały prawidłowo załadowane!";
    }

    const QIcon appIcon(":/resources/icons/wavelength_logo_upscaled.png");
    QApplication::setWindowIcon(appIcon);

    app.setStyle(QStyleFactory::create("Fusion"));

    CyberpunkStyle::ApplyStyle();

    // app.setPalette(darkPalette);
    // app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a2a2a; border: 1px solid #767676; }");

    QMainWindow window;
    window.setWindowTitle("Wavelength");
    // window.setStyleSheet("QMainWindow { background-color: #2d2d2d; }");

    auto *navbar = new Navbar(&window);
    window.addToolBar(Qt::TopToolBarArea, navbar);
    window.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    window.setContextMenuPolicy(Qt::NoContextMenu);

    const auto centralWidget = new QWidget(&window);
    const auto mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto stackedWidget = new AnimatedStackedWidget(centralWidget);
    stackedWidget->SetDuration(600);
    stackedWidget->SetAnimationType(AnimatedStackedWidget::Slide);
    mainLayout->addWidget(stackedWidget);

    auto animationWidget = new QWidget(stackedWidget);
    const auto animationLayout = new QVBoxLayout(animationWidget);
    animationLayout->setContentsMargins(0, 0, 0, 0);
    animationLayout->setSpacing(0);

    auto *animation = new BlobAnimation(animationWidget);
    animationLayout->addWidget(animation);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4); // Włącz MSAA dla wygładzania krawędzi
    animation->setFormat(format);

    stackedWidget->addWidget(animationWidget);

    auto chatView = new WavelengthChatView(stackedWidget);
    stackedWidget->addWidget(chatView);

    auto settingsView = new SettingsView(stackedWidget);
    stackedWidget->addWidget(settingsView);

    stackedWidget->setCurrentWidget(animationWidget);

    window.setCentralWidget(centralWidget);

    auto titleLabel = new QLabel("WAVELENGTH", animation);
    titleLabel->setFont(QFont("Blender Pro Heavy", 40, QFont::Bold));
    titleLabel->setAlignment(Qt::AlignCenter);

    // Aby uzyskać efekt obramowania z półprzezroczystym wypełnieniem,
    // będziemy musieli użyć stylów CSS bardziej kreatywnie
    titleLabel->setStyleSheet(
        "QLabel {"
        "   font-family: 'Blender Pro Heavy';"
        "   letter-spacing: 8px;"
        "   color: #ffffff;" // Zmień kolor tekstu na biały, zamiast ciemnego półprzezroczystego
        "   background-color: transparent;"
        "   border: 2px solid #e0b0ff;" // Jasnofioletowy neonowy border
        "   border-radius: 8px;"
        "   padding: 10px 20px;"
        "   text-transform: uppercase;"
        "}"
    );

    updateTitleLabelStyle(titleLabel, config->GetTitleTextColor(), config->GetTitleBorderColor());

    // Dodaj mocniejszy efekt poświaty dla cyberpunkowego wyglądu
    auto glowEffect = new QGraphicsDropShadowEffect(titleLabel);
    glowEffect->setBlurRadius(15);
    glowEffect->setColor(QColor("#e0b0ff"));
    glowEffect->setOffset(0, 0);
    titleLabel->setGraphicsEffect(glowEffect);

    titleLabel->raise();

    auto eventFilter = new ResizeEventFilter(titleLabel, animation);

    auto *textEffect = new CyberpunkTextEffect(titleLabel, animation);

    window.setMinimumSize(800, 600);
    window.setMaximumSize(1600, 900);
    window.resize(1024, 768);

    const auto instanceManager = new AppInstanceManager(&window, animation, &window);
    // instanceManager->Start();

    QObject::connect(instanceManager, &AppInstanceManager::instanceConnected, [&window]() {
    window.setAttribute(Qt::WA_TransparentForMouseEvents, true);
    window.setEnabled(false);
});


    WavelengthSessionCoordinator *coordinator = WavelengthSessionCoordinator::GetInstance();
    coordinator->Initialize();

    auto toggleEventListening = [animation, eventFilter](const bool enable) {
        if (enable) {
            // Włącz nasłuchiwanie eventów
            animation->installEventFilter(eventFilter);
            qDebug() << "Włączono nasłuchiwanie eventów animacji";
        } else {
            // Wyłącz nasłuchiwanie eventów
            animation->removeEventFilter(eventFilter);
            qDebug() << "Wyłączono nasłuchiwanie eventów animacji";
        }
    };

    toggleEventListening(true);


    QObject::connect(config, &WavelengthConfig::configChanged,
                     [animation, config, titleLabel, glowEffect](const QString& key) { // <<< Dodaj titleLabel, glowEffect
        qDebug() << "Lambda for configChanged executed with key:" << key;
        if (key == "background_color" || key == "all") {
            QColor newColor = config->GetBackgroundColor();
            qDebug() << "Config changed: background_color to" << newColor.name();
            QMetaObject::invokeMethod(animation, "setBackgroundColor", Qt::QueuedConnection, Q_ARG(QColor, newColor));
        }
        else if (key == "blob_color" || key == "all") {
            QColor newColor = config->GetBlobColor();
            qDebug() << "Config changed: blob_color to" << newColor.name();
            QMetaObject::invokeMethod(animation, "setBlobColor", Qt::QueuedConnection, Q_ARG(QColor, newColor));
        }
        // --- NOWE: Obsługa siatki ---
        else if (key == "grid_color" || key == "all") {
            QColor newColor = config->GetGridColor();
            qDebug() << "Config changed: grid_color to" << newColor.name(QColor::HexArgb);
            QMetaObject::invokeMethod(animation, "setGridColor", Qt::QueuedConnection, Q_ARG(QColor, newColor));
        }
        else if (key == "grid_spacing" || key == "all") {
            int newSpacing = config->GetGridSpacing();
            qDebug() << "Config changed: grid_spacing to" << newSpacing;
            QMetaObject::invokeMethod(animation, "setGridSpacing", Qt::QueuedConnection, Q_ARG(int, newSpacing));
        }
        // --- NOWE: Obsługa tytułu ---
        else if (key == "title_text_color" || key == "title_border_color" || key == "all") {
             // Aktualizuj styl, jeśli zmienił się kolor tekstu lub ramki
             QColor textColor = config->GetTitleTextColor();
             QColor borderColor = config->GetTitleBorderColor();
             qDebug() << "Config changed: Updating title style (Text:" << textColor.name() << ", Border:" << borderColor.name() << ")";
             // Wywołaj w głównym wątku dla bezpieczeństwa UI
             QMetaObject::invokeMethod(qApp, [titleLabel, textColor, borderColor](){ // Użyj qApp lub innego QObject z głównego wątku
                 updateTitleLabelStyle(titleLabel, textColor, borderColor);
             }, Qt::QueuedConnection);
        }
        else if (key == "title_glow_color" || key == "all") {
            QColor newColor = config->GetTitleGlowColor();
            qDebug() << "Config changed: title_glow_color to" << newColor.name();
            if (glowEffect) {
                // Wywołaj w głównym wątku dla bezpieczeństwa UI
                QMetaObject::invokeMethod(qApp, [glowEffect, newColor](){
                    glowEffect->setColor(newColor);
                }, Qt::QueuedConnection);
            }
        }
        // ---------------------------
    });

    QObject::connect(stackedWidget, &AnimatedStackedWidget::currentChanged,
    [stackedWidget, animationWidget, animation](const int index) {
        static int lastIndex = -1;
        if (lastIndex == index) return;
        lastIndex = index;

        const QWidget *currentWidget = stackedWidget->widget(index);
        if (currentWidget == animationWidget) {
            // Pokazujemy bloba tylko gdy przechodzimy DO widoku animacji
            QTimer::singleShot(stackedWidget->GetDuration(), [animation]() {
                animation->showAnimation();
            });
        }
    });

    auto switchToChatView = [chatView, stackedWidget, animation, navbar](const QString &frequency) {
        animation->hideAnimation();
        navbar->SetChatMode(true);
        chatView->SetWavelength(frequency, "");
        stackedWidget->SlideToWidget(chatView);
    };

    QObject::connect(coordinator, &WavelengthSessionCoordinator::messageReceived,
                     chatView, &WavelengthChatView::OnMessageReceived);

    QObject::connect(coordinator, &WavelengthSessionCoordinator::messageSent,
                     chatView, &WavelengthChatView::OnMessageSent);

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthClosed,
                     chatView, &WavelengthChatView::OnWavelengthClosed);

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthCreated,
                     [switchToChatView](const QString &frequency) {
                         qDebug() << "Wavelength created signal received";
                         switchToChatView(frequency);
                     });

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthJoined,
                     [switchToChatView](const QString &frequency) {
                         qDebug() << "Wavelength joined signal received";
                         switchToChatView(frequency);
                     });

    QObject::connect(chatView, &WavelengthChatView::wavelengthAborted,
    [stackedWidget, animationWidget, animation, titleLabel, textEffect, navbar]() {
        qDebug() << "Wavelength aborted, switching back to animation view";

        navbar->SetChatMode(false);
        animation->hideAnimation();
        animation->ResetLifeColor();
        stackedWidget->SlideToWidget(animationWidget);

        QTimer::singleShot(stackedWidget->GetDuration(), [animation, textEffect, titleLabel]() {
            animation->showAnimation();
            animation->ResetVisualization();
            titleLabel->adjustSize(); // Wymuś przeliczenie rozmiaru
            // Dodaj minimalne opóźnienie przed centrowaniem
            QTimer::singleShot(0, [titleLabel, animation]() {
                centerLabel(titleLabel, animation);
            });
            textEffect->StartAnimation(); // Rozpocznij animację tekstu od razu
        });
    });

    // Dodaj połączenie z nowym sygnałem dla synchronizacji pozycji tekstu
    QObject::connect(animation, &BlobAnimation::visualizationReset,
                     [titleLabel, animation]() {
                         // Ponownie wycentruj etykietę po zresetowaniu wizualizacji
                         centerLabel(titleLabel, animation);
                     });

    QObject::connect(navbar, &Navbar::settingsClicked, [stackedWidget, settingsView, animation]() {
    qDebug() << "Settings button clicked";

    animation->hideAnimation();

    animation->PauseAllEventTracking();

    stackedWidget->SlideToWidget(settingsView);
});

    QObject::connect(settingsView, &SettingsView::backToMainView,
[stackedWidget, animationWidget, animation, titleLabel, textEffect]() {
    qDebug() << "Back from settings, switching to animation view";

    animation->hideAnimation();
    animation->ResetLifeColor();
    stackedWidget->SlideToWidget(animationWidget);

    QTimer::singleShot(stackedWidget->GetDuration(), [animation, textEffect, titleLabel]() {
        animation->showAnimation();
        animation->ResetVisualization();
        titleLabel->adjustSize(); // Wymuś przeliczenie rozmiaru
        // Dodaj minimalne opóźnienie przed centrowaniem
        QTimer::singleShot(0, [titleLabel, animation]() {
            centerLabel(titleLabel, animation);
        });
        textEffect->StartAnimation(); // Rozpocznij animację tekstu od razu
    });
});

    QObject::connect(navbar, &Navbar::createWavelengthClicked, [&window, animation, coordinator, navbar]() {
        qDebug() << "Create wavelength button clicked";

        navbar->PlayClickSound();

        animation->SetLifeColor(QColor(0, 200, 0));

        WavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            const QString frequency = dialog.GetFrequency();
            const bool isPasswordProtected = dialog.IsPasswordProtected();
            const QString password = dialog.GetPassword();

            if (coordinator->CreateWavelength(frequency, isPasswordProtected, password)) {
                qDebug() << "Created and joined wavelength:" << frequency << "Hz";
            } else {
                qDebug() << "Failed to create wavelength";
                animation->ResetLifeColor();
            }
        } else {
            animation->ResetLifeColor();
        }
    });

    QObject::connect(navbar, &Navbar::joinWavelengthClicked, [&window, animation, coordinator, navbar]() {
        qDebug() << "Join wavelength button clicked";
        navbar->PlayClickSound();

        animation->SetLifeColor(QColor(0, 0, 200));

        JoinWavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            const QString frequency = dialog.GetFrequency();
            const QString password = dialog.GetPassword();

            if (coordinator->JoinWavelength(frequency, password)) {
                qDebug() << "Attempting to join wavelength:" << frequency << "Hz";
            } else {
                qDebug() << "Failed to join wavelength";
                animation->ResetLifeColor();
            }
        } else {
            animation->ResetLifeColor();
        }
    });

    if (!instanceManager->IsCreator()) {
        window.setWindowTitle("Wavelength - Sub Instance");
    }

    ShortcutManager* shortcutMgr = ShortcutManager::GetInstance();
    shortcutMgr->RegisterShortcuts(&window);    // Rejestruje skróty dla QMainWindow (Create, Join, Settings)
    shortcutMgr->RegisterShortcuts(chatView);   // Rejestruje skróty dla WavelengthChatView
    shortcutMgr->RegisterShortcuts(settingsView); // Rejestruje skróty dla SettingsVie

#ifdef Q_OS_WINDOWS
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10) {
        // Windows 10 1809>=
        const auto hwnd = reinterpret_cast<HWND>(window.winId());

        constexpr BOOL darkMode = TRUE;

        DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));
    }
#endif

    window.show();

    QTimer::singleShot(500, [titleLabel, animation, textEffect]() {
        titleLabel->setText("WAVELENGTH");
        titleLabel->adjustSize();
        // Dodaj minimalne opóźnienie przed centrowaniem
        QTimer::singleShot(0, [titleLabel, animation]() {
            centerLabel(titleLabel, animation);
        });
        titleLabel->show();

        QTimer::singleShot(300, [textEffect]() {
            textEffect->StartAnimation();
        });
    });

    const int exitCode = app.exec();

    return exitCode;
}