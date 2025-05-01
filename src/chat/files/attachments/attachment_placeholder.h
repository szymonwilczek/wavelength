#ifndef ATTACHMENT_PLACEHOLDER_H
#define ATTACHMENT_PLACEHOLDER_H

#include <qfuture.h>
#include <QScrollArea>
#include <QWidget>
#include <QWindow>
#include <QEvent>
#include <functional>

#include "attachment_queue_manager.h"
#include "../gif/player/inline_gif_player.h"
#include "../video/player/video_player_overlay.h"
#include "../audio/player/inline_audio_player.h"

// Forward declarations
class CyberAttachmentViewer;
class AutoScalingAttachment;
class InlineImageViewer;

/**
 * @brief A widget that acts as a placeholder for chat attachments before they are loaded.
 *
 * This class displays information about an attachment (filename, type icon) and provides
 * a mechanism to load and display the actual content (image, GIF, audio, video).
 * It handles fetching data (either directly provided or via AttachmentDataStore),
 * decoding, selecting the appropriate viewer widget, and managing the loading state (progress, errors).
 * It also provides functionality to show attachments in a full-size dialog.
 */
class AttachmentPlaceholder final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs an AttachmentPlaceholder widget.
     * Initializes the UI elements (labels, button, container) and sets up connections.
     * Automatically triggers loading after a short delay.
     * @param filename The original filename of the attachment.
     * @param type The general type of the attachment (e.g., "image", "audio", "video", "gif"). Used for icon selection.
     * @param parent Optional parent widget.
     */
    explicit AttachmentPlaceholder(const QString& filename, const QString& type, QWidget* parent = nullptr);

    /**
     * @brief Sets the actual content widget to be displayed within the placeholder.
     * Replaces any existing content, hides loading indicators, makes the content visible,
     * adjusts layout and size policies, and emits attachmentLoaded() after a short delay.
     * @param content The QWidget containing the loaded attachment content (e.g., ImageViewer, GifPlayer).
     */
    void SetContent(QWidget* content);

    /**
     * @brief Provides a size hint for the placeholder widget.
     * Returns a base size if no content is loaded, otherwise calculates a size based on the
     * content's size hint plus space for labels/buttons.
     * @return The recommended size for the widget.
     */
    QSize sizeHint() const override;

    /**
     * @brief Sets the reference ID and MIME type for an attachment whose data is stored in AttachmentDataStore.
     * @param attachment_id The unique ID referencing the data in AttachmentDataStore.
     * @param mime_type The MIME type of the attachment (e.g., "image/png", "audio/mpeg").
     */
    void SetAttachmentReference(const QString& attachment_id, const QString& mime_type);

    /**
     * @brief Sets the attachment data directly as a base64 encoded string.
     * @param base64_data The base64 encoded attachment data.
     * @param mime_type The MIME type of the attachment.
     */
    void SetBase64Data(const QString& base64_data, const QString& mime_type);

    /**
     * @brief Updates the UI elements to reflect the loading state.
     * Disables/enables the load button and shows/hides the progress label with appropriate text.
     * @param loading True if loading is in progress, false otherwise.
     */
    void SetLoading(bool loading) const;


signals:
    /**
     * @brief Emitted after the attachment content has been successfully loaded, set, and the layout updated.
     */
    void attachmentLoaded();

private slots:
    /**
     * @brief Slot triggered when the load/retry button is clicked or automatically after initialization.
     * Initiates the attachment loading process by adding a task to the AttachmentQueueManager.
     * Determines whether to fetch data from AttachmentDataStore or use directly provided base64 data.
     */
    void onLoadButtonClicked();

public slots:
    /**
     * @brief Updates the UI to display an error message.
     * Enables the retry button and shows the error message in the progress label.
     * @param error_msg The error message to display.
     */
    void SetError(const QString& error_msg);

    /**
     * @brief Creates and shows a non-modal dialog to display the full-size attachment (image or GIF).
     * Uses InlineImageViewer or InlineGifPlayer within a QScrollArea.
     * @param data The raw byte data of the attachment.
     * @param is_gif True if the data represents a GIF, false otherwise (assumed image).
     */
    void ShowFullSizeDialog(const QByteArray& data, bool is_gif);

    /**
     * @brief Adjusts the size and position of the full-size dialog and shows it.
     * Calculates the preferred dialog size based on the original content size,
     * limits it to the available screen geometry, centers it, and makes it visible.
     * Sets the content widget to its original fixed size to ensure scrollbars appear correctly if needed.
     * @param dialog The QDialog instance to adjust and show.
     * @param scroll_area The QScrollArea containing the content.
     * @param content_widget The widget displaying the actual content (image/GIF).
     * @param original_content_size The original, unscaled size of the content widget.
     */
    void AdjustAndShowDialog(QDialog* dialog, const QScrollArea* scroll_area, QWidget* content_widget, QSize original_content_size) const;

    /**
     * @brief Creates and displays a CyberAttachmentViewer containing an image.
     * Wraps an InlineImageViewer in an AutoScalingAttachment to handle thumbnail scaling and click-to-enlarge.
     * @param data The raw byte data of the image.
     */
    void ShowCyberImage(const QByteArray& data);

    /**
     * @brief Creates and displays a CyberAttachmentViewer containing a GIF.
     * Wraps an InlineGifPlayer in an AutoScalingAttachment to handle thumbnail scaling and click-to-enlarge.
     * @param data The raw byte data of the GIF.
     */
    void ShowCyberGif(const QByteArray& data);

    /**
     * @brief Creates and displays a CyberAttachmentViewer containing an audio player.
     * Uses InlineAudioPlayer.
     * @param data The raw byte data of the audio file.
     */
    void ShowCyberAudio(const QByteArray& data);

    /**
     * @brief Creates and displays a CyberAttachmentViewer containing a video preview (thumbnail + play button).
     * Clicking the preview opens a VideoPlayerOverlay dialog.
     * @param data The raw byte data of the video file.
     */
    void ShowCyberVideo(const QByteArray& data);

    /**
     * @brief Generates a thumbnail image from the first frame of a video in a background thread.
     * Uses VideoDecoder to extract the frame and updates the provided QLabel with the thumbnail.
     * Adds a play icon overlay to the thumbnail.
     * @param video_data The raw byte data of the video.
     * @param thumbnail_label The QLabel widget to display the generated thumbnail.
     */
    void GenerateThumbnail(const QByteArray& video_data, QLabel* thumbnail_label);

private:
    /** @brief The original filename of the attachment. */
    QString filename_;
    /** @brief Stores the base64 encoded data if provided directly. */
    QString base64_data_;
    /** @brief The MIME type of the attachment (e.g., "image/png"). */
    QString mime_type_;
    /** @brief Label displaying the attachment icon and filename. */
    QLabel* info_label_;
    /** @brief Label displaying loading progress or error messages. */
    QLabel* progress_label_;
    /** @brief Button to initiate loading or retry on error. */
    QPushButton* load_button_;
    /** @brief Container widget holding the loaded attachment content. */
    QWidget* content_container_;
    /** @brief Layout for the content_container_. */
    QVBoxLayout* content_layout_;
    /** @brief Flag indicating if the attachment content has been successfully loaded and set. */
    bool is_loaded_;
    /** @brief Temporary storage for video data when opening the player overlay. */
    QByteArray video_data_;
    /** @brief Pointer to the QLabel used for the video thumbnail preview. */
    QLabel* thumbnail_label_ = nullptr;
    /** @brief Function object (lambda) storing the action to perform when the video thumbnail is clicked. */
    std::function<void()> ClickHandler_;
    /** @brief Stores the attachment ID if data is referenced from AttachmentDataStore. */
    QString attachment_id_;
    /** @brief Flag indicating if attachment_id_ refers to data in AttachmentDataStore. */
    bool has_reference_ = false;

    /**
     * @brief Filters events, specifically handling mouse clicks on the video thumbnail label.
     * @param watched The object being watched.
     * @param event The event being processed.
     * @return True if the event was handled (thumbnail clicked), false otherwise.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

    /**
     * @brief Emits the attachmentLoaded signal after ensuring layout updates.
     * Called internally after SetContent completes its delayed updates.
     */
    void NotifyLoaded();
};

#endif //ATTACHMENT_PLACEHOLDER_H