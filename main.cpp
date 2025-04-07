#include <QOperatingSystemVersion>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

#include <QApplication>
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
    animation->setBackgroundColor(QColor(35, 35, 35));
    animation->setGridColor(QColor(60, 60, 60));
    animation->setGridSpacing(20);
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

    // Dodaj mocniejszy efekt poświaty dla cyberpunkowego wyglądu
    QGraphicsDropShadowEffect *glowEffect = new QGraphicsDropShadowEffect(titleLabel);
    glowEffect->setBlurRadius(15);
    glowEffect->setColor(QColor("#e0b0ff"));
    glowEffect->setOffset(0, 0);
    titleLabel->setGraphicsEffect(glowEffect);

    titleLabel->raise();

    // Zmodyfikuj filtr zdarzeń dla nowej warstwy
    ResizeEventFilter *eventFilter = new ResizeEventFilter(titleLabel, animation);

    auto *textEffect = new CyberpunkTextEffect(titleLabel, animation);

    window.setMinimumSize(800, 600);
    window.setMaximumSize(1600, 900);
    window.resize(1024, 768);

    AppInstanceManager *instanceManager = new AppInstanceManager(&window, animation, &window);
    instanceManager->start();


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

    QObject::connect(stackedWidget, &AnimatedStackedWidget::currentChanged,
                     [stackedWidget, animationWidget, toggleEventListening](int index) {
                         // Sprawdź, czy aktywny widget to animationWidget
                         QWidget *currentWidget = stackedWidget->widget(index);
                         toggleEventListening(currentWidget == animationWidget);
                     });

    auto switchToChatView = [chatView, stackedWidget, animation](const double frequency) {
        animation->pauseAllEventTracking();

        chatView->setWavelength(frequency, "");
        stackedWidget->slideToWidget(chatView);
        chatView->show();
    };

    QObject::connect(coordinator, &WavelengthSessionCoordinator::messageReceived,
                     chatView, &WavelengthChatView::onMessageReceived);

    QObject::connect(coordinator, &WavelengthSessionCoordinator::messageSent,
                     chatView, &WavelengthChatView::onMessageSent);

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthClosed,
                     chatView, &WavelengthChatView::onWavelengthClosed);

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthCreated,
                     [switchToChatView](double frequency) {
                         qDebug() << "Wavelength created signal received";
                         switchToChatView(frequency);
                     });

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthJoined,
                     [switchToChatView](double frequency) {
                         qDebug() << "Wavelength joined signal received";
                         switchToChatView(frequency);
                     });

    QObject::connect(chatView, &WavelengthChatView::wavelengthAborted,
                 [stackedWidget, animationWidget, animation, titleLabel, textEffect]() {
                     qDebug() << "Wavelength aborted, switching back to animation view";

                     animation->resetLifeColor();

                     // Najpierw przełączamy widok
                     stackedWidget->slideToWidget(animationWidget);

                     // Po zakończeniu animacji przywracamy śledzenie eventów
                     QTimer::singleShot(600, [animation, textEffect]() {
                         animation->resumeAllEventTracking();
                         textEffect->startAnimation();
                     });
                 });

    QObject::connect(navbar, &Navbar::createWavelengthClicked, [&window, animation, coordinator]() {
        qDebug() << "Create wavelength button clicked";

        animation->setLifeColor(QColor(0, 200, 0));

        WavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            double frequency = dialog.getFrequency();
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
            double frequency = dialog.getFrequency();
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
        // Upewnij się, że etykieta jest widoczna
        titleLabel->setText("WAVELENGTH");
        titleLabel->adjustSize();
        centerLabel(titleLabel, animation);
        titleLabel->show();

        // Rozpocznij animację z większym opóźnieniem
        QTimer::singleShot(300, [textEffect]() {
            textEffect->startAnimation();
        });
    });

    return app.exec();
}
