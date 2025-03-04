#ifndef NAVBAR_H
#define NAVBAR_H

#include <QToolBar>
#include <QPushButton>
#include <QPropertyAnimation>

class Navbar : public QToolBar {
    Q_OBJECT

public:
    explicit Navbar(QWidget *parent = nullptr);

    public slots:
        void toggleNavbar();

private:
    QPushButton *createWavelengthButton;
    QPushButton *joinWavelengthButton;
    QPropertyAnimation *animation;
    bool isExpanded;
};

#endif // NAVBAR_H