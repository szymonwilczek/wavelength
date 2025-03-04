#include <QApplication>
#include <QMainWindow>
#include "dot_animation.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Animacja gęstej zawiesiny");

    auto *animation = new DotAnimation();
    window.setCentralWidget(animation);
    window.resize(500, 500);
    window.show();

    return app.exec();
}
