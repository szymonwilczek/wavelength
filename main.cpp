#include <QApplication>
#include <QMainWindow>
#include <QPalette>
#include <QStyleFactory>
#include "navbar.h"
#include "blobanimation.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>

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

    auto *animation = new BlobAnimation(centralWidget);
    animation->setBackgroundColor(QColor(35, 35, 35));
    animation->setGridColor(QColor(60, 60, 60));
    animation->setGridSpacing(20);

    mainLayout->addWidget(animation);
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
    window.show();

    QTimer::singleShot(10, [titleLabel, outlineLabel, animation]() {
        centerLabel(titleLabel, animation);
        centerLabel(outlineLabel, animation);
    });

    return app.exec();
}