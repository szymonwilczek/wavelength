#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include "dot_animation.h"
#include "navbar.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Animacja gęstej zawiesiny");

    auto *animation = new DotAnimation();
    window.setCentralWidget(animation);

    auto *navbar = new Navbar(&window);
    window.addToolBar(Qt::LeftToolBarArea, navbar);

    auto *toggleButton = new QPushButton("☰", &window);
    toggleButton->setStyleSheet("QPushButton { background: rgba(50, 50, 50, 0.8); color: white; }");
    toggleButton->setGeometry(10, 10, 30, 30);

    QObject::connect(toggleButton, &QPushButton::clicked, navbar, &Navbar::toggleNavbar);

    window.resize(500, 500);
    window.show();

    return app.exec();
}