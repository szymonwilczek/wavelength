#ifndef NAVBAR_H
#define NAVBAR_H

#include <QToolBar>
#include <QHBoxLayout>
#include <QSoundEffect>
#include <QSpacerItem>

#include "../../ui/buttons/cyberpunk_button.h"
#include "network_status_widget.h"

/**
 * @brief A custom QToolBar serving as the main navigation bar for the application.
 *
 * This Navbar displays the application logo, network status, and action buttons
 * ("Generate Wavelength", "Merge Wavelength", "Settings"). It features a cyberpunk
 * aesthetic with custom styling and glow effects on the logo and buttons.
 * The layout dynamically adjusts when entering/exiting a chat mode using SetChatMode,
 * hiding the action buttons and repositioning the network status widget.
 * It also plays a click sound when buttons are pressed.
 */
class Navbar final : public QToolBar {
    Q_OBJECT

public:
    /**
     * @brief Constructs the Navbar.
     * Initializes the toolbar's appearance and behavior (non-movable, non-floatable),
     * sets up the layout, creates and styles the logo, network status widget, action buttons,
     * and corner decorative elements. Connects button signals and initializes the click sound effect.
     * @param parent Optional parent widget.
     */
    explicit Navbar(QWidget *parent = nullptr);

public slots:
    /**
     * @brief Switches the Navbar layout between normal mode and chat mode.
     * In chat mode, the action buttons and right corner element are hidden, and the
     * network status widget is aligned to the right. In normal mode, all elements are visible
     * and the network status widget is centered.
     * @param inChat True to enter chat mode, false to return to normal mode.
     */
    void SetChatMode(bool inChat) const;

    /**
     * @brief Plays a predefined click sound effect.
     * Triggered when any of the main action buttons are clicked.
     */
    void PlayClickSound() const;

protected:
    /**
     * @brief Overridden context menu event handler.
     * Ignores the event to prevent the default toolbar context menu from appearing.
     * @param event The context menu event.
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:
    /** @brief Emitted when the "Generate Wavelength" button is clicked. */
    void createWavelengthClicked();
    /** @brief Emitted when the "Merge Wavelength" button is clicked. */
    void joinWavelengthClicked();
    /** @brief Emitted when the "Settings" button is clicked. */
    void settingsClicked();

private:
    /** @brief The main horizontal layout managing all elements within the Navbar. */
    QHBoxLayout* main_layout_;
    /** @brief Container widget holding the left corner element and the logo label. */
    QWidget* logo_container_;
    /** @brief Label displaying the "WAVELENGTH" logo text with glow effect. */
    QLabel *logo_label_;
    /** @brief Widget displaying the current network connection status. */
    NetworkStatusWidget* network_status_;
    /** @brief Container widget holding the action buttons (Create, Join, Settings). */
    QWidget* buttons_container_;
    /** @brief Decorative label element placed at the far right in normal mode. */
    QLabel* corner_element_;

    /** @brief Button to trigger the creation of a new wavelength. */
    CyberpunkButton *create_button_;
    /** @brief Button to trigger joining an existing wavelength. */
    CyberpunkButton *join_button_;
    /** @brief Button to open the application settings. */
    CyberpunkButton *settings_button_;

    /** @brief Spacer item used to push elements apart in the main layout (left side). */
    QSpacerItem* spacer1_;
    /** @brief Spacer item used to push elements apart in the main layout (right side). Its stretch factor changes in chat mode. */
    QSpacerItem* spacer2_;

    /** @brief Sound effect played when action buttons are clicked. */
    QSoundEffect* click_sound_;
};

#endif // NAVBAR_H