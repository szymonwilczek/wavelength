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
#include "wavelength/wavelength_manager.h"

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

    WavelengthManager* wavelengthManager = WavelengthManager::getInstance();

    QObject::connect(&app, &QApplication::aboutToQuit, [wavelengthManager]() {
        int activeFreq = wavelengthManager->getActiveWavelength();
        if (activeFreq > 0) {
            wavelengthManager->closeWavelength(activeFreq);
            qDebug() << "Closing wavelength server on application exit:" << activeFreq;
        }
    });

    QObject::connect(navbar, &Navbar::createWavelengthClicked, [&window, animation, chatView, stackedWidget]() {
        qDebug() << "Create wavelength button clicked";

        animation->setLifeColor(QColor(0, 200, 0));

        WavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            int frequency = dialog.getFrequency();
            QString name = dialog.getName();
            bool isPasswordProtected = dialog.isPasswordProtected();
            QString password = dialog.getPassword();

            WavelengthManager* manager = WavelengthManager::getInstance();
            if (manager->createWavelength(frequency, name, isPasswordProtected, password)) {
                chatView->setWavelength(frequency, name);
                stackedWidget->setCurrentWidget(chatView);
                chatView->show();
                qDebug() << "Created and joined wavelength:" << frequency << "Hz";
            } else {
                qDebug() << "Failed to create wavelength";
                animation->resetLifeColor();
            }
        } else {
            animation->resetLifeColor();
        }
    });

    QObject::connect(navbar, &Navbar::joinWavelengthClicked, [&window, chatView, stackedWidget]() {
        qDebug() << "Join wavelength button clicked";

        JoinWavelengthDialog dialog(&window);
        if (dialog.exec() == QDialog::Accepted) {
            int frequency = dialog.getFrequency();
            QString password = dialog.getPassword();

            WavelengthManager* manager = WavelengthManager::getInstance();
            if (!manager->isFrequencyAvailable(frequency)) {
                if (manager->joinWavelength(frequency, password)) {
                    chatView->setWavelength(frequency);
                    stackedWidget->setCurrentWidget(chatView);
                    chatView->show();
                    qDebug() << "Joined wavelength:" << frequency << "Hz";
                } else {
                    qDebug() << "Failed to join wavelength";
                }
            } else {
                qDebug() << "Wavelength does not exist on frequency:" << frequency;
            }
        }
    });

    QObject::connect(chatView, &WavelengthChatView::wavelengthAborted, [stackedWidget, animationWidget, animation]() {
        qDebug() << "Wavelength aborted, returning to main view";
        stackedWidget->setCurrentWidget(animationWidget);
        animation->resetLifeColor();
    });

    QObject::connect(wavelengthManager, &WavelengthManager::wavelengthClosed, [stackedWidget, animationWidget, animation, chatView, wavelengthManager](int frequency) {
        if (wavelengthManager->getActiveWavelength() == frequency) {
            qDebug() << "Current wavelength was closed by host, returning to main view";
            stackedWidget->setCurrentWidget(animationWidget);
            animation->resetLifeColor();
            QTimer::singleShot(100, [chatView]() {
                chatView->hide();
            });
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