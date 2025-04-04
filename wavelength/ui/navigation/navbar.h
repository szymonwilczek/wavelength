#ifndef NAVBAR_H
#define NAVBAR_H

#include <QToolBar>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>
#include <QResizeEvent>

class CyberpunkButton;

class Navbar : public QToolBar {
    Q_OBJECT

public:
    explicit Navbar(QWidget *parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    signals:
        void createWavelengthClicked();
    void joinWavelengthClicked();

private:
    QLabel *logoLabel;
    CyberpunkButton *createWavelengthButton;
    CyberpunkButton *joinWavelengthButton;
};

#endif // NAVBAR_H