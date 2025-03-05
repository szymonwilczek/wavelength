#ifndef NAVBAR_H
#define NAVBAR_H

#include <QToolBar>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QContextMenuEvent>

class Navbar : public QToolBar {
    Q_OBJECT

public:
    explicit Navbar(QWidget *parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QPushButton *createWavelengthButton;
    QPushButton *joinWavelengthButton;
    QLabel *logoLabel;
};

#endif // NAVBAR_H