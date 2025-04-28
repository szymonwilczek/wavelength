#ifndef NAVBAR_H
#define NAVBAR_H

#include <QToolBar>
#include <QHBoxLayout>
#include <QSoundEffect>
#include <QSpacerItem>

#include "../../ui/buttons/cyberpunk_button.h"
#include "network_status_widget.h"

class Navbar final : public QToolBar {
    Q_OBJECT

public:
    explicit Navbar(QWidget *parent = nullptr);

public slots:
    void setChatMode(const bool inChat) const;
    void playClickSound() const;

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    signals:
        void createWavelengthClicked();
    void joinWavelengthClicked();
    void settingsClicked();

private:
    QHBoxLayout* m_mainLayout; // Wskaźnik do głównego layoutu
    QWidget* m_logoContainer;
    QLabel *logoLabel;
    NetworkStatusWidget* m_networkStatus; // Wskaźnik do widgetu statusu sieci
    QWidget* m_buttonsContainer; // Wskaźnik do kontenera przycisków
    QLabel* m_cornerElement2; // Wskaźnik do prawego elementu narożnego

    CyberpunkButton *createWavelengthButton;
    CyberpunkButton *joinWavelengthButton;
    CyberpunkButton *settingsButton;

    QSpacerItem* m_spacer1; // Pierwszy element rozciągający
    QSpacerItem* m_spacer2; // Drugi element rozciągający

    QSoundEffect* m_clickSound;
};

#endif // NAVBAR_H