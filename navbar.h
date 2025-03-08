#ifndef NAVBAR_H
#define NAVBAR_H

#include <QToolBar>
#include <QPushButton>
#include <QLabel>

class Navbar : public QToolBar {
    Q_OBJECT

public:
    explicit Navbar(QWidget *parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    signals:
        void createWavelengthClicked();
    void joinWavelengthClicked();

private:
    QLabel *logoLabel;
    QPushButton *createWavelengthButton;
    QPushButton *joinWavelengthButton;
};

#endif // NAVBAR_H