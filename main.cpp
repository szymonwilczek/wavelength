#include <QOperatingSystemVersion>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

#include <QApplication>
#include <QCoreApplication>
#include <qguiapplication.h>
#include <QMainWindow>
#include "src/ui/navigation/navbar.h"
#include "src/blob/core/blob_animation.h"
#include <QLabel>
#include <QSoundEffect>
#include <QTimer>
#include <QStyleFactory>
#include <QCommandLineParser>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>

#include "src/app/wavelength_config.h"
#include "src/app/managers/font_manager.h"
#include "src/app/managers/app_instance_manager.h"
#include "src/ui/dialogs/join_wavelength_dialog.h"
#include "src/ui/views/chat_view.h"
#include "src/ui/dialogs/create_wavelength_dialog.h"

#include "src/session/session_coordinator.h"
#include "src/app/style/cyberpunk_style.h"
#include "src/ui/effects/cyberpunk_text_effect.h"
#include "src/ui/widgets/animated_stacked_widget.h"
#include "src/app/managers/shortcut_manager.h"
#include "src/app/managers/translation_manager.h"
#include "src/ui/views/settings_view.h"
#include "src/util/resize_event_filter.h"
#include "src/util/wavelength_utilities.h"


int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Wavelength");
    QCoreApplication::setApplicationName("WavelengthApp");

    auto boot_sound = new QSoundEffect(&app);
    boot_sound->setSource(QUrl("qrc:/resources/sounds/interface/boot_up.wav"));
    boot_sound->setVolume(1.0);

    auto shutdown_sound = new QSoundEffect(&app);
    shutdown_sound->setSource(QUrl("qrc:/resources/sounds/interface/shutdown.wav"));
    shutdown_sound->setVolume(1.0);


    WavelengthConfig *config = WavelengthConfig::GetInstance();

    TranslationManager *translator = TranslationManager::GetInstance();
    if (!translator->Initialize(config->GetLanguageCode())) {
        qCritical() << "[MAIN] Failed to initialize translation manager. Fallback to English.";
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("Wavelength Application");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption override_option("run-override",
                                             "Internal flag to start the system override sequence immediately.");
    parser.addOption(override_option);
    parser.process(app);

    if (parser.isSet(override_option)) {
        qDebug() << "--run-override flag detected.";
#ifdef Q_OS_WIN
        if (!SystemOverrideManager::IsRunningAsAdmin()) {
            qWarning() << "[MAIN] Override sequence requires administrator privileges. Attempting relaunch...";
            if (SystemOverrideManager::RelaunchAsAdmin(app.arguments())) {
                qDebug() << "[MAIN] Relaunch successful. Exiting current instance.";
                return 0;
            }
            qCritical() << "[MAIN] Failed to relaunch as administrator. Override aborted.";
            QMessageBox::critical(nullptr, translator->Translate("MainWindow.IsNotRunningAdminTitle", "Admin Privileges Required"), translator->Translate("MainWindow.IsNotRunningAdminMessage", "Failed to relaunch with administrator privileges. The override sequence cannot continue."));
            return 1;
        }
        qDebug() << "[MAIN] Running with administrator privileges.";
        SystemOverrideManager override_manager;
        QObject::connect(&override_manager, &SystemOverrideManager::overrideFinished, &app, &QCoreApplication::quit);
        override_manager.InitiateOverrideSequence(false);
        return QApplication::exec();
#else
        qWarning() << "--run-override flag ignored on non-Windows OS.";
#endif
    }

    QObject::connect(boot_sound, &QSoundEffect::statusChanged, [boot_sound] {
        if (boot_sound->status() == QSoundEffect::Ready) {
            boot_sound->play();
            QObject::disconnect(boot_sound, &QSoundEffect::statusChanged, nullptr, nullptr);
        } else if (boot_sound->status() == QSoundEffect::Error) {
            qDebug() << "[MAIN] Error loading boot sound. Source:" << boot_sound->source();
            QObject::disconnect(boot_sound, &QSoundEffect::statusChanged, nullptr, nullptr);
        }
    });

    if (boot_sound->status() == QSoundEffect::Error) {
        qDebug() << "[MAIN] Initial error loading boot sound. Source:" << boot_sound->source();
    } else if (boot_sound->status() == QSoundEffect::Ready) {
        boot_sound->play();
        QObject::disconnect(boot_sound, &QSoundEffect::statusChanged, nullptr, nullptr);
    }

    QObject::connect(&app, &QApplication::aboutToQuit, [shutdown_sound] {
        if (shutdown_sound->isLoaded()) {
            shutdown_sound->play();
            QTimer::singleShot(2500, &QCoreApplication::quit);
        } else {
            qDebug() << "[MAIN] Shutdown sound not loaded when quitting.";
        }
    });

    FontManager *font_manager = FontManager::GetInstance();
    if (!font_manager->Initialize()) {
        qWarning() << "[MAIN] WARNING: Not all fonts loaded correctly!";
    }

    const QFont defaultFont(font_manager->GetFont("BlenderPro", 10, QFont::Bold));
    QApplication::setFont(defaultFont);

    const QIcon app_icon(":/resources/icons/wavelength_logo_upscaled.png");
    QApplication::setWindowIcon(app_icon);

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    CyberpunkStyle::ApplyStyle();

    QMainWindow window;
    window.setWindowTitle("Wavelength");

    auto *navbar = new Navbar(&window);
    window.addToolBar(Qt::TopToolBarArea, navbar);
    window.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    window.setContextMenuPolicy(Qt::NoContextMenu);

    const auto central_widget = new QWidget(&window);
    const auto main_layout = new QVBoxLayout(central_widget);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    auto stacked_widget = new AnimatedStackedWidget(central_widget);
    stacked_widget->SetDuration(600);
    stacked_widget->SetAnimationType(AnimatedStackedWidget::Slide);
    main_layout->addWidget(stacked_widget);

    auto animation_widget = new QWidget(stacked_widget);
    const auto animation_layout = new QVBoxLayout(animation_widget);
    animation_layout->setContentsMargins(0, 0, 0, 0);
    animation_layout->setSpacing(0);

    auto *animation = new BlobAnimation(animation_widget);
    animation_layout->addWidget(animation);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4); // MSAA
    animation->setFormat(format);

    stacked_widget->addWidget(animation_widget);

    auto chat_view = new ChatView(stacked_widget);
    stacked_widget->addWidget(chat_view);

    auto settings_view = new SettingsView(stacked_widget);
    stacked_widget->addWidget(settings_view);

    stacked_widget->setCurrentWidget(animation_widget);

    window.setCentralWidget(central_widget);

    auto title_label = new QLabel("WAVELENGTH", animation);
    title_label->setFont(QFont("Blender Pro Heavy", 40, QFont::Bold));
    title_label->setAlignment(Qt::AlignCenter);

    title_label->setStyleSheet(
        "QLabel {"
        "   font-family: 'Blender Pro Heavy';"
        "   letter-spacing: 8px;"
        "   color: #ffffff;"
        "   background-color: transparent;"
        "   border: 2px solid #e0b0ff;"
        "   border-radius: 8px;"
        "   padding: 10px 20px;"
        "   text-transform: uppercase;"
        "}"
    );

    WavelengthUtilities::UpdateTitleLabelStyle(title_label, config->GetTitleTextColor(), config->GetTitleBorderColor());

    auto glow_effect = new QGraphicsDropShadowEffect(title_label);
    glow_effect->setBlurRadius(15);
    glow_effect->setColor(QColor("#e0b0ff"));
    glow_effect->setOffset(0, 0);
    title_label->setGraphicsEffect(glow_effect);

    title_label->raise();

    auto event_filter = new ResizeEventFilter(title_label, animation);

    auto *text_effect = new CyberpunkTextEffect(title_label, animation);

    window.setMinimumSize(1200, 900);
    window.setMaximumSize(1600, 900);
    window.resize(1024, 768);

    const auto instance_manager = new AppInstanceManager(&window, animation, &window);
    // instance_manager->Start();

    QObject::connect(instance_manager, &AppInstanceManager::instanceConnected, [&window] {
        window.setAttribute(Qt::WA_TransparentForMouseEvents, true);
        window.setEnabled(false);
    });


    SessionCoordinator *coordinator = SessionCoordinator::GetInstance();
    coordinator->Initialize();

    auto event_listening = [animation, event_filter](const bool enable) {
        if (enable) {
            animation->installEventFilter(event_filter);
        } else {
            animation->removeEventFilter(event_filter);
        }
    };

    event_listening(true);


    QObject::connect(config, &WavelengthConfig::configChanged,
                     [animation, config, title_label, glow_effect](const QString &key) {
                         if (key == "background_color" || key == "all") {
                             QColor new_color = config->GetBackgroundColor();
                             QMetaObject::invokeMethod(animation, "setBackgroundColor", Qt::QueuedConnection,
                                                       Q_ARG(QColor, new_color));
                         } else if (key == "blob_color" || key == "all") {
                             QColor new_color = config->GetBlobColor();
                             QMetaObject::invokeMethod(animation, "setBlobColor", Qt::QueuedConnection,
                                                       Q_ARG(QColor, new_color));
                         } else if (key == "grid_color" || key == "all") {
                             QColor new_color = config->GetGridColor();
                             QMetaObject::invokeMethod(animation, "setGridColor", Qt::QueuedConnection,
                                                       Q_ARG(QColor, new_color));
                         } else if (key == "grid_spacing" || key == "all") {
                             int new_spacing = config->GetGridSpacing();
                             QMetaObject::invokeMethod(animation, "setGridSpacing", Qt::QueuedConnection,
                                                       Q_ARG(int, new_spacing));
                         } else if (key == "title_text_color" || key == "title_border_color" || key == "all") {
                             QColor text_color = config->GetTitleTextColor();
                             QColor border_color = config->GetTitleBorderColor();
                             QMetaObject::invokeMethod(qApp, [title_label, text_color, border_color] {
                                 WavelengthUtilities::UpdateTitleLabelStyle(title_label, text_color, border_color);
                             }, Qt::QueuedConnection);
                         } else if (key == "title_glow_color" || key == "all") {
                             QColor new_color = config->GetTitleGlowColor();
                             if (glow_effect) {
                                 QMetaObject::invokeMethod(qApp, [glow_effect, new_color] {
                                     glow_effect->setColor(new_color);
                                 }, Qt::QueuedConnection);
                             }
                         }
                     });

    QObject::connect(stacked_widget, &AnimatedStackedWidget::currentChanged,
                     [stacked_widget, animation_widget, animation](const int index) {
                         static int last_index = -1;
                         if (last_index == index) return;
                         last_index = index;

                         const QWidget *current_widget = stacked_widget->widget(index);
                         if (current_widget == animation_widget) {
                             QTimer::singleShot(stacked_widget->GetDuration(), [animation] {
                                 animation->showAnimation();
                             });
                         }
                     });

    // ReSharper disable once CppDFAUnreachableFunctionCall
    auto switch_to_chat_view = [chat_view, stacked_widget, animation, navbar](const QString &frequency) {
        animation->hideAnimation();
        navbar->SetChatMode(true);
        chat_view->SetWavelength(frequency, "");
        stacked_widget->SlideToWidget(chat_view);
    };

    QObject::connect(coordinator, &SessionCoordinator::messageReceived,
                     chat_view, &ChatView::OnMessageReceived);

    QObject::connect(coordinator, &SessionCoordinator::messageSent,
                     chat_view, &ChatView::OnMessageSent);

    QObject::connect(coordinator, &SessionCoordinator::wavelengthClosed,
                     chat_view, &ChatView::OnWavelengthClosed);

    QObject::connect(coordinator, &SessionCoordinator::wavelengthCreated,
                     [switch_to_chat_view](const QString &frequency) {
                         switch_to_chat_view(frequency);
                     });

    QObject::connect(coordinator, &SessionCoordinator::wavelengthJoined,
                     [switch_to_chat_view](const QString &frequency) {
                         switch_to_chat_view(frequency);
                     });

    QObject::connect(chat_view, &ChatView::wavelengthAborted,
                     [stacked_widget, animation_widget, animation, title_label, text_effect, navbar] {
                         navbar->SetChatMode(false);
                         animation->hideAnimation();
                         animation->ResetLifeColor();
                         stacked_widget->SlideToWidget(animation_widget);

                         QTimer::singleShot(stacked_widget->GetDuration(), [animation, text_effect, title_label] {
                             animation->showAnimation();
                             animation->ResetVisualization();
                             title_label->adjustSize();
                             QTimer::singleShot(0, [title_label, animation] {
                                 WavelengthUtilities::CenterLabel(title_label, animation);
                             });
                             text_effect->StartAnimation();
                         });
                     });

    QObject::connect(animation, &BlobAnimation::visualizationReset,
                     [title_label, animation] {
                         WavelengthUtilities::CenterLabel(title_label, animation);
                     });

    QObject::connect(navbar, &Navbar::settingsClicked, [stacked_widget, settings_view, animation] {
        animation->hideAnimation();

        animation->PauseAllEventTracking();

        stacked_widget->SlideToWidget(settings_view);
    });

    QObject::connect(settings_view, &SettingsView::backToMainView,
                     [stacked_widget, animation_widget, animation, title_label, text_effect] {
                         animation->hideAnimation();
                         animation->ResetLifeColor();
                         stacked_widget->SlideToWidget(animation_widget);

                         QTimer::singleShot(stacked_widget->GetDuration(), [animation, text_effect, title_label] {
                             animation->showAnimation();
                             animation->ResetVisualization();
                             title_label->adjustSize();
                             QTimer::singleShot(0, [title_label, animation] {
                                 WavelengthUtilities::CenterLabel(title_label, animation);
                             });
                             text_effect->StartAnimation();
                         });
                     });

    QObject::connect(navbar, &Navbar::createWavelengthClicked, [&window, animation, coordinator, navbar] {
        navbar->PlayClickSound();

        animation->SetLifeColor(QColor(0, 200, 0));

        CreateWavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            const QString frequency = dialog.GetFrequency();
            const bool is_password_protected = dialog.IsPasswordProtected();
            const QString password = dialog.GetPassword();

            if (coordinator->CreateWavelength(frequency, is_password_protected, password)) {
            } else {
                qDebug() << "[MAIN] Failed to create wavelength";
                animation->ResetLifeColor();
            }
        } else {
            animation->ResetLifeColor();
        }
    });

    QObject::connect(navbar, &Navbar::joinWavelengthClicked, [&window, animation, coordinator, navbar] {
        navbar->PlayClickSound();

        animation->SetLifeColor(QColor(0, 0, 200));

        JoinWavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            const QString frequency = dialog.GetFrequency();
            const QString password = dialog.GetPassword();

            if (coordinator->JoinWavelength(frequency, password)) {
            } else {
                qDebug() << "[MAIN] Failed to join wavelength";
                animation->ResetLifeColor();
            }
        } else {
            animation->ResetLifeColor();
        }
    });

    if (!instance_manager->IsCreator()) {
        window.setWindowTitle(translator->Translate("InstanceManager.NotCreatorTitle", "Wavelength - Sub Instance"));
    }

    ShortcutManager *shortcut_manager = ShortcutManager::GetInstance();
    shortcut_manager->RegisterShortcuts(&window);
    shortcut_manager->RegisterShortcuts(chat_view);
    shortcut_manager->RegisterShortcuts(settings_view);

#ifdef Q_OS_WINDOWS
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10) {
        // Windows 10 1809>=
        const auto hwnd = reinterpret_cast<HWND>(window.winId());
        constexpr BOOL dark_mode = TRUE;

        DwmSetWindowAttribute(hwnd, 20, &dark_mode, sizeof(dark_mode));
    }
#endif

    window.show();

    QTimer::singleShot(500, [title_label, animation, text_effect] {
        title_label->setText("WAVELENGTH");
        title_label->adjustSize();
        QTimer::singleShot(0, [title_label, animation] {
            WavelengthUtilities::CenterLabel(title_label, animation);
        });
        title_label->show();

        QTimer::singleShot(300, [text_effect] {
            text_effect->StartAnimation();
        });
    });

    const int exit_code = app.exec();

    return exit_code;
}
