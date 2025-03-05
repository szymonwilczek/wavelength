#include <QApplication>
#include <QMainWindow>
#include "navbar.h"
#include "blobanimation.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Pk4");

    auto *animation = new BlobAnimation(&window);
    window.setCentralWidget(animation);

    auto *navbar = new Navbar(&window);
    window.addToolBar(Qt::LeftToolBarArea, navbar);

    auto *toggleButton = new QPushButton("☰", &window);
    toggleButton->setStyleSheet("QPushButton { background: rgba(50, 50, 50, 0.8); color: white; }");
    toggleButton->setGeometry(10, 10, 30, 30);

    QObject::connect(toggleButton, &QPushButton::clicked, navbar, &Navbar::toggleNavbar);

    window.setMinimumSize(800, 600);

    window.resize(1024, 768);

    window.show();

    return app.exec();
}