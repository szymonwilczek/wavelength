#ifndef AUTO_SCALING_ATTACHMENT_H
#define AUTO_SCALING_ATTACHMENT_H

#include <QScreen>

#include "../gif/player/inline_gif_player.h"
#include "../image/displayer/image_viewer.h"

/**
 * @brief A wrapper widget that automatically scales its content (e.g., image, GIF)
 *        to fit within a maximum allowed size, while preserving aspect ratio.
 *
 * This widget takes another widget (content) and displays it. If the content's
 * original size exceeds the specified maximum size, the content is scaled down
 * proportionally. A label indicating "Click to enlarge" is shown over the scaled
 * content when the mouse hovers over it. Clicking the widget emits the clicked() signal,
 * typically used to show the content in its original size in a separate viewer.
 */
class AutoScalingAttachment final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs an AutoScalingAttachment widget.
     * Sets up the layout, installs event filters, connects signals for content loading,
     * and triggers initial scaling check.
     * @param content The widget containing the attachment content to be scaled.
     * @param parent Optional parent widget.
     */
    explicit AutoScalingAttachment(QWidget* content, QWidget* parent = nullptr);

    /**
     * @brief Sets the maximum allowed size for the content.
     * If the content's original size exceeds this, it will be scaled down.
     * Triggers a re-evaluation of the scaling.
     * @param max_size The maximum QSize allowed for the content.
     */
    void SetMaxAllowedSize(const QSize& max_size);

    /**
     * @brief Gets the original, unscaled size of the content widget.
     * Primarily uses the content's sizeHint().
     * @return The original QSize of the content, or an invalid QSize if undetermined.
     */
    QSize ContentOriginalSize() const;

    /**
     * @brief Checks if the content is currently scaled down.
     * @return True if the content is scaled, false otherwise.
     */
    bool IsScaled() const {
        return is_scaled_;
    }

    /**
     * @brief Gets a pointer to the original content widget.
     * @return Pointer to the content QWidget.
     */
    QWidget* Content() const {
        return content_;
    }

    /**
     * @brief Provides a size hint for the AutoScalingAttachment widget.
     * Returns the current (potentially scaled) size of the content container.
     * @return The recommended QSize for the widget.
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Emitted when the user clicks on the attachment container.
     * Typically used to trigger showing the full-size attachment.
     */
    void clicked();

protected:
    /**
     * @brief Filters events for the content container, specifically capturing mouse clicks.
     * Emits the clicked() signal when the content container is left-clicked.
     * @param watched The object being watched (should be content_container_).
     * @param event The event being processed.
     * @return True if the event was a left-click on the container (event handled), false otherwise.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

    /**
     * @brief Handles mouse enter events.
     * Shows the "Click to enlarge" label if the content is scaled.
     * @param event The enter event.
     */
    void enterEvent(QEvent* event) override;

    /**
     * @brief Handles mouse leave events.
     * Hides the "Click to enlarge" label.
     * @param event The leave event.
     */
    void leaveEvent(QEvent* event) override;

    /**
     * @brief Handles resize events for the widget.
     * Updates the position of the "Click to enlarge" label if it's visible.
     * @param event The resize event.
     */
    void resizeEvent(QResizeEvent* event) override;

private slots:
    /**
     * @brief Checks the content's original size against the maximum allowed size and applies scaling if necessary.
     * Calculates the target scaled size, sets the fixed size of the content container,
     * updates the is_scaled_ flag, and adjusts the UI accordingly (e.g., info label visibility).
     * Triggers layout updates.
     */
    void CheckAndScaleContent();

private:
    /**
     * @brief Updates the position of the info_label_ ("Click to enlarge") to be centered at the bottom of the content container.
     */
    void UpdateInfoLabelPosition() const;

    /** @brief Pointer to the original widget displaying the attachment content. */
    QWidget* content_;
    /** @brief Container widget that holds the content_ and whose size is adjusted during scaling. */
    QWidget* content_container_;
    /** @brief Label displayed over scaled content on hover ("Click to enlarge"). */
    QLabel* info_label_;
    /** @brief Flag indicating whether the content is currently scaled down. */
    bool is_scaled_;
    /** @brief The current scaled size of the content_container_. */
    QSize scaled_size_;
    /** @brief The maximum size allowed for the content before scaling is applied. */
    QSize max_allowed_size_;
};

#endif // AUTO_SCALING_ATTACHMENT_H