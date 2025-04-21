#include <QOperatingSystemVersion>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

#include <QMainWindow>
#include "wavelength/ui/navigation/navbar.h"
#include "blob/core/blob_animation.h"
#include <QLabel>
#include <QTimer>

#include "font_manager.h"
#include "blob/core/app_instance_manager.h"
#include "wavelength/dialogs/join_wavelength_dialog.h"
#include "wavelength/view/wavelength_chat_view.h"
#include "wavelength/dialogs/wavelength_dialog.h"

#include "wavelength/session/wavelength_session_coordinator.h"
#include "wavelength/ui/cyberpunk_style.h"
#include "wavelength/view/main_window/cyberpunk_text_effect.h"
#include "wavelength/ui/widgets/animated_stacked_widget.h"
#include "wavelength/view/settings_view.h"
#include "wavelength/view/settings/tabs/classified_tab/components/system_override_manager.h"

void centerLabel(QLabel *label, BlobAnimation *animation) {
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
    QString style = QString(
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
    ).arg(textColor.name(QColor::HexRgb))
     .arg(borderColor.name(QColor::HexRgb));
    label->setStyleSheet(style);
    qDebug() << "Updated titleLabel style:" << style;
}

class ResizeEventFilter : public QObject {
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

    WavelengthConfig *config = WavelengthConfig::getInstance();

    // --- Przetwarzanie argumentów linii poleceń ---
    QCommandLineParser parser;
    parser.setApplicationDescription("Wavelength Application");
    parser.addHelpOption();
    parser.addVersionOption();

    // Dodaj opcję dla sekwencji override
    QCommandLineOption overrideOption("run-override", "Internal flag to start the system override sequence immediately.");
    parser.addOption(overrideOption);

    parser.process(app);
    // ---------------------------------------------

    // --- Sprawdź, czy uruchomić sekwencję override ---
    if (parser.isSet(overrideOption)) {
        qDebug() << "--run-override flag detected.";

#ifdef Q_OS_WIN
        // Sprawdź, czy faktycznie mamy uprawnienia administratora
        if (!SystemOverrideManager::isRunningAsAdmin()) {
            qCritical() << "Relaunched with --run-override but not running as admin! Aborting.";
            QMessageBox::critical(nullptr, "Administrator Privileges Required",
                                  "Failed to start the override sequence because administrator privileges are missing even after relaunch.");
            return 1; // Zakończ z błędem
        }
        qDebug() << "Confirmed running as admin. Initiating override sequence...";

        // Utwórz menedżera override i zainicjuj sekwencję
        SystemOverrideManager overrideManager; // Utwórz na stosie, bo nie potrzebujemy głównego okna
        overrideManager.initiateOverrideSequence(false); // false, bo to nie pierwsze uruchomienie

        // Nie tworzymy głównego okna, tylko uruchamiamy pętlę zdarzeń
        // Menedżer override sam pokaże swoją animację
        return app.exec(); // Uruchom pętlę zdarzeń dla animacji override

#else
        // Na innych systemach, po prostu kontynuuj (brak implementacji UAC)
        qWarning() << "--run-override flag ignored on non-Windows OS.";
        // Kontynuuj normalne uruchomienie poniżej
#endif
    }

    FontManager* fontManager = FontManager::getInstance();
    if (!fontManager->initialize()) {
        qWarning() << "Uwaga: Nie wszystkie czcionki zostały prawidłowo załadowane!";
    }

    const QIcon appIcon(":/assets/icons/wavelength_logo_upscaled.png");
    QApplication::setWindowIcon(appIcon);

    app.setStyle(QStyleFactory::create("Fusion"));

    CyberpunkStyle::applyStyle();

    // app.setPalette(darkPalette);
    // app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a2a2a; border: 1px solid #767676; }");

    QMainWindow window;
    window.setWindowTitle("Wavelength");
    // window.setStyleSheet("QMainWindow { background-color: #2d2d2d; }");

    auto *navbar = new Navbar(&window);
    window.addToolBar(Qt::TopToolBarArea, navbar);
    window.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    window.setContextMenuPolicy(Qt::NoContextMenu);

    QWidget *centralWidget = new QWidget(&window);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    AnimatedStackedWidget *stackedWidget = new AnimatedStackedWidget(centralWidget);
    stackedWidget->setDuration(600);
    stackedWidget->setAnimationType(AnimatedStackedWidget::Slide);
    mainLayout->addWidget(stackedWidget);

    QWidget *animationWidget = new QWidget(stackedWidget);
    QVBoxLayout *animationLayout = new QVBoxLayout(animationWidget);
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

    WavelengthChatView *chatView = new WavelengthChatView(stackedWidget);
    stackedWidget->addWidget(chatView);

    SettingsView *settingsView = new SettingsView(stackedWidget);
    stackedWidget->addWidget(settingsView);

    stackedWidget->setCurrentWidget(animationWidget);

    window.setCentralWidget(centralWidget);

    QLabel *titleLabel = new QLabel("WAVELENGTH", animation);
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

    updateTitleLabelStyle(titleLabel, config->getTitleTextColor(), config->getTitleBorderColor());

    // Dodaj mocniejszy efekt poświaty dla cyberpunkowego wyglądu
    QGraphicsDropShadowEffect *glowEffect = new QGraphicsDropShadowEffect(titleLabel);
    glowEffect->setBlurRadius(15);
    glowEffect->setColor(QColor("#e0b0ff"));
    glowEffect->setOffset(0, 0);
    titleLabel->setGraphicsEffect(glowEffect);

    titleLabel->raise();

    ResizeEventFilter *eventFilter = new ResizeEventFilter(titleLabel, animation);

    auto *textEffect = new CyberpunkTextEffect(titleLabel, animation);

    window.setMinimumSize(800, 600);
    window.setMaximumSize(1600, 900);
    window.resize(1024, 768);

    const auto instanceManager = new AppInstanceManager(&window, animation, &window);
    instanceManager->start();

    QObject::connect(instanceManager, &AppInstanceManager::instanceConnected, [&window]() {
    window.setAttribute(Qt::WA_TransparentForMouseEvents, true);
    window.setEnabled(false);
});


    WavelengthSessionCoordinator *coordinator = WavelengthSessionCoordinator::getInstance();
    coordinator->initialize();

    auto toggleEventListening = [animation, eventFilter](bool enable) {
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
            QColor newColor = config->getBackgroundColor();
            qDebug() << "Config changed: background_color to" << newColor.name();
            QMetaObject::invokeMethod(animation, "setBackgroundColor", Qt::QueuedConnection, Q_ARG(QColor, newColor));
        }
        else if (key == "blob_color" || key == "all") {
            QColor newColor = config->getBlobColor();
            qDebug() << "Config changed: blob_color to" << newColor.name();
            QMetaObject::invokeMethod(animation, "setBlobColor", Qt::QueuedConnection, Q_ARG(QColor, newColor));
        }
        // --- NOWE: Obsługa siatki ---
        else if (key == "grid_color" || key == "all") {
            QColor newColor = config->getGridColor();
            qDebug() << "Config changed: grid_color to" << newColor.name(QColor::HexArgb);
            QMetaObject::invokeMethod(animation, "setGridColor", Qt::QueuedConnection, Q_ARG(QColor, newColor));
        }
        else if (key == "grid_spacing" || key == "all") {
            int newSpacing = config->getGridSpacing();
            qDebug() << "Config changed: grid_spacing to" << newSpacing;
            QMetaObject::invokeMethod(animation, "setGridSpacing", Qt::QueuedConnection, Q_ARG(int, newSpacing));
        }
        // --- NOWE: Obsługa tytułu ---
        else if (key == "title_text_color" || key == "title_border_color" || key == "all") {
             // Aktualizuj styl, jeśli zmienił się kolor tekstu lub ramki
             QColor textColor = config->getTitleTextColor();
             QColor borderColor = config->getTitleBorderColor();
             qDebug() << "Config changed: Updating title style (Text:" << textColor.name() << ", Border:" << borderColor.name() << ")";
             // Wywołaj w głównym wątku dla bezpieczeństwa UI
             QMetaObject::invokeMethod(qApp, [titleLabel, textColor, borderColor](){ // Użyj qApp lub innego QObject z głównego wątku
                 updateTitleLabelStyle(titleLabel, textColor, borderColor);
             }, Qt::QueuedConnection);
        }
        else if (key == "title_glow_color" || key == "all") {
            QColor newColor = config->getTitleGlowColor();
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
    [stackedWidget, animationWidget, animation](int index) {
        static int lastIndex = -1;
        if (lastIndex == index) return;
        lastIndex = index;

        QWidget *currentWidget = stackedWidget->widget(index);
        if (currentWidget == animationWidget) {
            // Pokazujemy bloba tylko gdy przechodzimy DO widoku animacji
            QTimer::singleShot(stackedWidget->duration(), [animation]() {
                animation->show();
            });
        }
    });

    auto switchToChatView = [chatView, stackedWidget, animation, navbar](const QString frequency) {
        animation->hide();
        navbar->setChatMode(true);
        chatView->setWavelength(frequency, "");
        stackedWidget->slideToWidget(chatView);
    };

    QObject::connect(coordinator, &WavelengthSessionCoordinator::messageReceived,
                     chatView, &WavelengthChatView::onMessageReceived);

    QObject::connect(coordinator, &WavelengthSessionCoordinator::messageSent,
                     chatView, &WavelengthChatView::onMessageSent);

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthClosed,
                     chatView, &WavelengthChatView::onWavelengthClosed);

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthCreated,
                     [switchToChatView](QString frequency) {
                         qDebug() << "Wavelength created signal received";
                         switchToChatView(frequency);
                     });

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthJoined,
                     [switchToChatView](QString frequency) {
                         qDebug() << "Wavelength joined signal received";
                         switchToChatView(frequency);
                     });

    QObject::connect(chatView, &WavelengthChatView::wavelengthAborted,
    [stackedWidget, animationWidget, animation, titleLabel, textEffect, navbar]() {
        qDebug() << "Wavelength aborted, switching back to animation view";

        navbar->setChatMode(false);
        animation->hide();
        animation->resetLifeColor();
        stackedWidget->slideToWidget(animationWidget);

        QTimer::singleShot(stackedWidget->duration(), [animation, textEffect, titleLabel]() {
            animation->show();
            animation->resetVisualization();
            // --- DODANO ---
            titleLabel->adjustSize(); // Wymuś przeliczenie rozmiaru
            // -------------
            centerLabel(titleLabel, animation);
            textEffect->startAnimation();
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

    animation->hide();

    animation->pauseAllEventTracking();

    stackedWidget->slideToWidget(settingsView);
});

    QObject::connect(settingsView, &SettingsView::backToMainView,
[stackedWidget, animationWidget, animation, titleLabel, textEffect]() {
    qDebug() << "Back from settings, switching to animation view";

    animation->hide();
    animation->resetLifeColor();
    stackedWidget->slideToWidget(animationWidget);

    QTimer::singleShot(stackedWidget->duration(), [animation, textEffect, titleLabel]() {
        animation->show();
        animation->resetVisualization();
        // --- DODANO ---
        titleLabel->adjustSize(); // Wymuś przeliczenie rozmiaru
        // -------------
        centerLabel(titleLabel, animation);
        textEffect->startAnimation();
    });
});

    QObject::connect(navbar, &Navbar::createWavelengthClicked, [&window, animation, coordinator]() {
        qDebug() << "Create wavelength button clicked";

        animation->setLifeColor(QColor(0, 200, 0));

        WavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            QString frequency = dialog.getFrequency();
            bool isPasswordProtected = dialog.isPasswordProtected();
            QString password = dialog.getPassword();

            if (coordinator->createWavelength(frequency, isPasswordProtected, password)) {
                qDebug() << "Created and joined wavelength:" << frequency << "Hz";
            } else {
                qDebug() << "Failed to create wavelength";
                animation->resetLifeColor();
            }
        } else {
            animation->resetLifeColor();
        }
    });

    QObject::connect(navbar, &Navbar::joinWavelengthClicked, [&window, animation, coordinator]() {
        qDebug() << "Join wavelength button clicked";

        animation->setLifeColor(QColor(0, 0, 200));

        JoinWavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            QString frequency = dialog.getFrequency();
            QString password = dialog.getPassword();

            if (coordinator->joinWavelength(frequency, password)) {
                qDebug() << "Attempting to join wavelength:" << frequency << "Hz";
            } else {
                qDebug() << "Failed to join wavelength";
                animation->resetLifeColor();
            }
        } else {
            animation->resetLifeColor();
        }
    });

    if (!instanceManager->isCreator()) {
        window.setWindowTitle("Wavelength - Sub Instance");
    }

#ifdef Q_OS_WINDOWS
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10) {
        // Windows 10 1809>=
        HWND hwnd = (HWND) window.winId();

        BOOL darkMode = TRUE;

        DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));
    }
#endif

    window.show();

    QTimer::singleShot(500, [titleLabel, animation, textEffect]() {
        titleLabel->setText("WAVELENGTH");
        titleLabel->adjustSize(); // Upewnij się, że rozmiar jest obliczony po ustawieniu tekstu
        centerLabel(titleLabel, animation);
        titleLabel->show();

        QTimer::singleShot(300, [textEffect]() {
            textEffect->startAnimation();
        });
    });

    return app.exec();
}