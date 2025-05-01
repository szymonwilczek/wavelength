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
    void SetChatMode(bool inChat) const;
    void PlayClickSound() const;

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    signals:
        void createWavelengthClicked();
        void joinWavelengthClicked();
        void settingsClicked();

private:
    QHBoxLayout* main_layout_; // Wskaźnik do głównego layoutu
    QWidget* logo_container_;
    QLabel *logo_label_;
    NetworkStatusWidget* network_status_; // Wskaźnik do widgetu statusu sieci
    QWidget* buttons_container_; // Wskaźnik do kontenera przycisków
    QLabel* corner_element_; // Wskaźnik do prawego elementu narożnego

    CyberpunkButton *create_button_;
    CyberpunkButton *join_button_;
    CyberpunkButton *settings_button_;

    QSpacerItem* spacer1_; // Pierwszy element rozciągający
    QSpacerItem* spacer2_; // Drugi element rozciągający

    QSoundEffect* click_sound_;
};

#endif // NAVBAR_H