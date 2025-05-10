#ifndef CYBER_ATTACHMENT_VIEWER_H
#define CYBER_ATTACHMENT_VIEWER_H

#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>

#include "../chat/effects/mask_overlay_effect.h"

class TranslationManager;
/**
 * @brief A widget that displays another widget (content) with a cyberpunk-themed decryption animation.
 *
 * This viewer wraps a content widget and initially hides it behind a MaskOverlay.
 * It simulates a scanning and decryption process:
 * 1. Initialization: Shows a status label and the MaskOverlay with a moving scanline.
 * 2. Scanning: Status label indicates scanning; MaskOverlay continues animating.
 * 3. Decryption: Status label shows decryption progress (0-100%); MaskOverlay reveals the content
 *    from top to bottom as the decryption progress increases (controlled by the decryptionCounter property).
 * 4. Finished: MaskOverlay is hidden, the actual content widget is shown, and the status label indicates completion.
 *
 * The viewer itself has a cyberpunk-styled border and AR markers drawn in its paintEvent.
 * It manages the lifecycle of the MaskOverlay and the visibility of the content widget.
 */
class CyberAttachmentViewer final : public QWidget {
    Q_OBJECT
    /** @brief Property controlling the decryption progress (0-100). Animatable. Drives the MaskOverlay reveal. */
    Q_PROPERTY(
        int decryptionCounter READ GetDecryptionCounter WRITE SetDecryptionCounter NOTIFY decryptionCounterChanged)
    // Added NOTIFY

public:
    /**
     * @brief Constructs a CyberAttachmentViewer.
     * Initializes the layout, status label, content container, MaskOverlay, animation timer,
     * and applies basic styling. Connects the decryptionCounter property to the MaskOverlay's progress.
     * @param parent Optional parent widget.
     */
    explicit CyberAttachmentViewer(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Ensures graphics effects are removed from the content widget if set.
     */
    ~CyberAttachmentViewer() override;

    /**
     * @brief Gets the current decryption progress counter (0-100).
     * @return The decryption counter-value.
     */
    int GetDecryptionCounter() const { return decryption_counter_; }

    /**
     * @brief Sets the decryption progress counter (0-100).
     * Updates the internal value, updates the status label text, and emits decryptionCounterChanged.
     * @param counter The new counter-value.
     */
    void SetDecryptionCounter(int counter);

    /**
     * @brief Forces an update of the content layout and geometry.
     * Useful after the content widget's size changes dynamically. Notifies parent widgets.
     */
    void UpdateContentLayout();

    /**
     * @brief Sets the widget to be displayed after the decryption animation.
     * Removes any previous content, adds the new content widget to the layout,
     * hides the content initially, shows and configures the MaskOverlay, resets the
     * decryption state, and updates geometry.
     * @param content Pointer to the widget to display.
     */
    void SetContent(QWidget *content);

    /**
     * @brief Returns the recommended size for the viewer based on its content and layout.
     * Calculates the size needed to accommodate the status label and the content widget's size hint.
     * @return The calculated QSize hint.
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Overridden resize event handler.
     * Ensures the MaskOverlay is resized to match the content container's bounds.
     * @param event The resize event.
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief Overridden paint event handler. Draws the custom cyberpunk border and AR markers.
     * Renders the clipped border, decorative lines, corner markers, and status text (SEC level, LOCK status).
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private slots:
    /**
     * @brief Slot triggered automatically after a short delay or potentially by a user action (though currently automatic).
     * Initiates the scanning animation if not already decrypted. If decrypted, closes the viewer.
     */
    void OnActionButtonClicked();

    /**
     * @brief Starts the visual scanning phase.
     * Updates the status label, ensures the MaskOverlay is visible and animating its scanline.
     * Schedules the start of the decryption animation after a delay.
     */
    void StartScanningAnimation();

    /**
     * @brief Starts the decryption phase.
     * Updates the status label, ensures the MaskOverlay is visible, and starts a QPropertyAnimation
     * to animate the decryptionCounter property from 0 to 100.
     */
    void StartDecryptionAnimation();

    /**
     * @brief Slot called by animation_timer_ (if active) to update visual elements during animation.
     * Adds random glitch characters to the status label text during decryption.
     */
    void UpdateAnimation() const;

    /**
     * @brief Updates the status label text to show the current decryption percentage.
     */
    void UpdateDecryptionStatus() const;

    /**
     * @brief Slot called when the decryption animation finishes.
     * Sets the state to decrypted, updates the status label, stops and hides the MaskOverlay,
     * and makes the actual content widget visible.
     */
    void FinishDecryption();

    /**
     * @brief Closes the viewer by emitting the viewingFinished signal.
     */
    void CloseViewer();

signals:
    /**
     * @brief Emitted when the decryptionCounter property changes.
     * Connected to the MaskOverlay's SetRevealProgress slot.
     * @param value The new decryption counter-value (0-100).
     */
    void decryptionCounterChanged(int value);

    /**
     * @brief Emitted when the viewer is closed (either manually or automatically after decryption).
     */
    void viewingFinished();

private:
    /** @brief Current decryption progress (0-100). */
    int decryption_counter_;
    /** @brief Flag indicating if the viewer is currently in the "scanning" phase. */
    bool is_scanning_;
    /** @brief Flag indicating if the decryption process has completed. */
    bool is_decrypted_;

    /** @brief Main layout for the viewer. */
    QVBoxLayout *layout_;
    /** @brief Container widget holding the content_widget_ and mask_overlay_. */
    QWidget *content_container_;
    /** @brief Layout for the content_container_. */
    QVBoxLayout *content_layout_;
    /** @brief Pointer to the widget being displayed (e.g., an image viewer). */
    QWidget *content_widget_ = nullptr;
    /** @brief Label displaying status messages (Scanning, Decrypting %, Finished). */
    QLabel *status_label_;
    /** @brief Timer potentially used for secondary animations (like status label glitches). */
    QTimer *animation_timer_;
    /** @brief The overlay widget that handles the scanline and reveal effect. */
    MaskOverlayEffect *mask_overlay_;
    /** @brief Pointer to the translation manager for handling UI translations. */
    TranslationManager *translator_ = nullptr;
};

#endif // CYBER_ATTACHMENT_VIEWER_H
