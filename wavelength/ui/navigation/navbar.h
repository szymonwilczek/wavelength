#ifndef NAVBAR_H
#define NAVBAR_H

#include <QToolBar>
#include <QLabel>
#include <QHBoxLayout>

class CyberpunkButton;

class Navbar : public QToolBar {
    Q_OBJECT

public:
    explicit Navbar(QWidget *parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    signals:
        void createWavelengthClicked();
    void joinWavelengthClicked();
    void settingsClicked();  // Nowy sygnał dla przycisku ustawień

private:
    QLabel *logoLabel;
    CyberpunkButton *createWavelengthButton;
    CyberpunkButton *joinWavelengthButton;
    CyberpunkButton *settingsButton;  // Nowy przycisk ustawień
};

#endif // NAVBAR_H