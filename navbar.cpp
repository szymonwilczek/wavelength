#include "navbar.h"
#include <QPushButton>

Navbar::Navbar(QWidget *parent) : QToolBar(parent), isExpanded(false) {
    setStyleSheet("QToolBar { background: rgba(0, 0, 0, 0.5); }");

    createWavelengthButton = new QPushButton("Generate Wavelength", this);
    joinWavelengthButton = new QPushButton("Merge Wavelength", this);
    createWavelengthButton->setStyleSheet("QPushButton { background: rgba(50, 50, 50, 0.8); color: white; }");
    joinWavelengthButton->setStyleSheet("QPushButton { background: rgba(50, 50, 50, 0.8); color: white; }");

    addWidget(createWavelengthButton);
    addWidget(joinWavelengthButton);

    setMovable(false);
    setFloatable(false);
    setAllowedAreas(Qt::LeftToolBarArea);

    animation = new QPropertyAnimation(this, "geometry");
    animation->setDuration(300);
}

void Navbar::toggleNavbar() {
    if (isExpanded) {
        animation->setStartValue(geometry());
        animation->setEndValue(QRect(-width(), y(), width(), height()));
    } else {
        animation->setStartValue(geometry());
        animation->setEndValue(QRect(0, y(), width(), height()));
    }
    animation->start();
    isExpanded = !isExpanded;
}