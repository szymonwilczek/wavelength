#include <QApplication>
#include <QMainWindow>
#include <QPalette>
#include <QStyleFactory>
#include "navbar.h"
#include "blob/core/blob_animation.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QStackedWidget>
#include <QTimer>

#include "blob/core/app_instance_manager.h"
#include "wavelength/join_wavelength_dialog.h"
#include "wavelength/wavelength_chat_view.h"
#include "wavelength/wavelength_dialog.h"

// Zamiast wavelength_manager.h, teraz importujemy koordynatora
#include "wavelength/session/wavelength_session_coordinator.h"

void centerLabel(QLabel* label, BlobAnimation* animation) {
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
    ResizeEventFilter(QLabel* label, BlobAnimation* animation)
        : QObject(animation), m_label(label), m_animation(animation) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == m_animation && event->type() == QEvent::Resize) {
            centerLabel(m_label, m_animation);
            return false;
        }
        return false;
    }

private:
    QLabel* m_label;
    BlobAnimation* m_animation;
};


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::WindowText, QColor(200, 200, 200));
    darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(55, 55, 55));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::ToolTipText, QColor(200, 200, 200));
    darkPalette.setColor(QPalette::Text, QColor(200, 200, 200));
    darkPalette.setColor(QPalette::Button, QColor(60, 60, 60));
    darkPalette.setColor(QPalette::ButtonText, QColor(200, 200, 200));
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    app.setPalette(darkPalette);
    app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a2a2a; border: 1px solid #767676; }");

    QMainWindow window;
    window.setWindowTitle("Pk4");
    window.setStyleSheet("QMainWindow { background-color: #2d2d2d; }");

    auto *navbar = new Navbar(&window);
    window.addToolBar(Qt::TopToolBarArea, navbar);
    window.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    window.setContextMenuPolicy(Qt::NoContextMenu);

    QWidget *centralWidget = new QWidget(&window);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QStackedWidget *stackedWidget = new QStackedWidget(centralWidget);
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

    stackedWidget->addWidget(animationWidget);

    WavelengthChatView *chatView = new WavelengthChatView(stackedWidget);
    stackedWidget->addWidget(chatView);

    stackedWidget->setCurrentWidget(animationWidget);

    window.setCentralWidget(centralWidget);

    QLabel *titleLabel = new QLabel("Hello, World!", animation);
    QLabel *outlineLabel = new QLabel("Hello, World!", animation);

    QFont titleFont("Arial", 40, QFont::Bold);
    titleFont.setStyleStrategy(QFont::PreferAntialias);
    titleLabel->setFont(titleFont);
    outlineLabel->setFont(titleFont);

    titleLabel->setStyleSheet("QLabel { color: #bbbbbb; letter-spacing: 2px; background-color: transparent; }");
    outlineLabel->setStyleSheet("QLabel { color: #555555; letter-spacing: 2px; background-color: transparent; }");

    titleLabel->setAlignment(Qt::AlignCenter);
    outlineLabel->setAlignment(Qt::AlignCenter);

    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(titleLabel);
    shadowEffect->setBlurRadius(15);
    shadowEffect->setColor(QColor(0, 0, 0, 200));
    shadowEffect->setOffset(2, 2);
    titleLabel->setGraphicsEffect(shadowEffect);

    outlineLabel->lower();

    ResizeEventFilter* eventFilter = new ResizeEventFilter(titleLabel, animation);
    ResizeEventFilter* outlineFilter = new ResizeEventFilter(outlineLabel, animation);

    animation->installEventFilter(eventFilter);
    animation->installEventFilter(outlineFilter);

    window.setMinimumSize(800, 600);
    window.setMaximumSize(1600, 900);
    window.resize(1024, 768);

    AppInstanceManager* instanceManager = new AppInstanceManager(&window, animation, &window);
    instanceManager->start();


    QObject::connect(instanceManager, &AppInstanceManager::absorptionStarted, [](const QString& targetId) {
        qDebug() << "[MAIN] Rozpoczęto absorpcję instancji:" << targetId;
    });

    QObject::connect(instanceManager, &AppInstanceManager::absorptionStarted, [animation, instanceManager](const QString& targetId) {
        if (targetId == instanceManager->getInstanceId()) {
            animation->startBeingAbsorbed();
        }
    });

    QObject::connect(instanceManager, &AppInstanceManager::instanceAbsorbed, [animation, instanceManager](const QString& targetId) {
        if (targetId == instanceManager->getInstanceId()) {
            animation->finishBeingAbsorbed();
        }
    });

    QObject::connect(instanceManager, &AppInstanceManager::absorptionCancelled, [animation, instanceManager](const QString& targetId) {
        if (targetId == instanceManager->getInstanceId()) {
            animation->cancelAbsorption();
        }
    });

    // Inicjalizacja koordynatora zamiast WavelengthManager
    WavelengthSessionCoordinator* coordinator = WavelengthSessionCoordinator::getInstance();
    coordinator->initialize();

    auto switchToChatView = [chatView, stackedWidget, animation](int frequency) {
        qDebug() << "Switching to chat view for frequency:" << frequency;
        chatView->setWavelength(frequency, "");
        stackedWidget->setCurrentWidget(chatView);
        chatView->show();
    };

    QObject::connect(coordinator, &WavelengthSessionCoordinator::messageReceived,
                chatView, [chatView](int frequency, const QString& message) {
    qDebug() << "Main: Received message event for frequency" << frequency;
    chatView->onMessageReceived(frequency, message);
});

    // Podłączanie sygnałów z koordynatora zamiast z managera
    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthCreated,
             [switchToChatView](int frequency) {
    qDebug() << "Wavelength created signal received";
    switchToChatView(frequency);
});

    QObject::connect(coordinator, &WavelengthSessionCoordinator::wavelengthJoined,
                    [switchToChatView](int frequency) {
        qDebug() << "Wavelength joined signal received";
        switchToChatView(frequency);
    });

    QObject::connect(chatView, &WavelengthChatView::wavelengthAborted, [stackedWidget, animationWidget, animation]() {
        qDebug() << "Wavelength aborted, switching back to animation view";
        animation->resetLifeColor(); // Resetuje kolor, jeśli był ustawiony
        stackedWidget->setCurrentWidget(animationWidget);
    });

    QObject::connect(navbar, &Navbar::createWavelengthClicked, [&window, animation, chatView, stackedWidget, coordinator]() {
    qDebug() << "Create wavelength button clicked";

    animation->setLifeColor(QColor(0, 200, 0));

    WavelengthDialog dialog(&window);
    if (dialog.exec() == QDialog::Accepted) {
        int frequency = dialog.getFrequency();
        QString name = dialog.getName();
        bool isPasswordProtected = dialog.isPasswordProtected();
        QString password = dialog.getPassword();

        // Używamy koordynatora zamiast managera
        if (coordinator->createWavelength(frequency, name, isPasswordProtected, password)) {
            qDebug() << "Created and joined wavelength:" << frequency << "Hz";
        } else {
            qDebug() << "Failed to create wavelength";
            animation->resetLifeColor();
        }
    } else {
        animation->resetLifeColor();
    }
});

    QObject::connect(navbar, &Navbar::joinWavelengthClicked, [&window, animation, chatView, stackedWidget, coordinator]() {
        qDebug() << "Join wavelength button clicked";

        animation->setLifeColor(QColor(0, 0, 200)); // Niebieski kolor dla dołączania

        JoinWavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            int frequency = dialog.getFrequency();
            QString password = dialog.getPassword();

            // Używamy koordynatora zamiast managera
            if (coordinator->joinWavelength(frequency, password)) {
                qDebug() << "Attempting to join wavelength:" << frequency << "Hz";
                // Nie przełączamy widoku tutaj - zrobi to signal handler
            } else {
                qDebug() << "Failed to join wavelength";
                animation->resetLifeColor();
            }
        } else {
            animation->resetLifeColor();
        }
    });

    if (!instanceManager->isCreator()) {
        window.setWindowTitle("Pk4 - Instancja podrzędna");
    }

    window.show();

    QTimer::singleShot(10, [titleLabel, outlineLabel, animation]() {
        centerLabel(titleLabel, animation);
        centerLabel(outlineLabel, animation);
    });

    return app.exec();
}