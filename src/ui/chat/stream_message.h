#ifndef STREAM_MESSAGE_H
#define STREAM_MESSAGE_H

#include <QSvgRenderer>
#include <qtimeline.h>
#include <utility>
#include <QApplication>
#include <QVBoxLayout>

#include "../chat/effects/long_text_display_effect.h"
#include "../chat/effects/text_display_effect.h"
#include "../../chat/files/attachments/attachment_placeholder.h"
#include "effects/electronic_shutdown_effect.h"

class AttachmentViewer;

/**
 * @brief A custom widget representing a single message within the CommunicationStream.
 *
 * This widget displays message content (text or attachment placeholder) with cyberpunk styling,
 * including background, borders, glow effects, and AR-style markers. It handles different
 * message types (Received, Transmitted, System) with distinct color schemes.
 * It supports animations for fading in/out, disintegration, and electronic shutdown.
 * For short messages, it uses CyberTextDisplay for a typing reveal effect.
 * For long messages, it uses CyberLongTextDisplay within a QScrollArea.
 * It also manages navigation buttons (Next/Previous) and a "Mark as Read" button.
 */
class StreamMessage final : public QWidget {
    Q_OBJECT
    /** @brief Property controlling the widget's overall opacity (0.0 to 1.0). Animatable. */
    Q_PROPERTY(qreal opacity READ GetOpacity WRITE SetOpacity)
    /** @brief Property controlling the intensity of the border glow effect (0.0 to 1.0+). Animatable. */
    Q_PROPERTY(qreal glowIntensity READ GetGlowIntensity WRITE SetGlowIntensity)
    /** @brief Property controlling the progress of the electronic shutdown effect (0.0 to 1.0). Animatable. */
    Q_PROPERTY(qreal shutdownProgress READ GetShutdownProgress WRITE SetShutdownProgress)

public:
    /**
     * @brief Enum defining the type/origin of the message, affecting its styling.
     */
    enum MessageType {
        kReceived, ///< Message received from another user (pink/purple theme).
        kTransmitted, ///< Message sent by the current user (blue/cyan theme).
        kSystem ///< System notification (yellow/orange theme).
    };

    Q_ENUM(MessageType)

    /** @brief Button used to mark the message as read and trigger the closing animation. */
    QPushButton *mark_read_button;

    /**
     * @brief Static utility function to create a QIcon from an SVG file, colored with the specified color.
     * @param svg_path Path to the SVG file.
     * @param color The desired color for the icon.
     * @param size The desired size of the output icon pixmap.
     * @return A QIcon containing the colored pixmap, or a null QIcon on failure.
     */
    static QIcon CreateColoredSvgIcon(const QString &svg_path, const QColor &color, const QSize &size);

    /**
     * @brief Constructs a StreamMessage widget.
     * Initializes UI elements (layout, text display/label/scroll area), sets up effects and animations,
     * cleans the content, and determines the display method based on content length and type.
     * @param content The message content (can be plain text or HTML with attachment placeholders).
     * @param sender The identifier of the message sender ("SYSTEM" for system messages).
     * @param type The type of the message (Received, Transmitted, System).
     * @param message_id Optional unique identifier, used for progress updates.
     * @param parent Optional parent widget.
     */
    explicit StreamMessage(QString content, QString sender, MessageType type, QString message_id = QString(),
                           QWidget *parent = nullptr);

    /**
     * @brief Updates the content of an existing message, typically used for progress updates.
     * Cleans the new content, updates the appropriate display widget (CyberTextDisplay, CyberLongTextDisplay, or QLabel),
     * and adjusts the widget size if necessary.
     * @param new_content The updated message content.
     */
    void UpdateContent(const QString &new_content);

    // --- Property Getters/Setters ---
    /** @brief Gets the current opacity level. */
    qreal GetOpacity() const { return opacity_; }

    /** @brief Sets the opacity level and updates the QGraphicsOpacityEffect. */
    void SetOpacity(qreal opacity);

    /** @brief Gets the current glow intensity level. */
    qreal GetGlowIntensity() const { return glow_intensity_; }

    /** @brief Sets the glow intensity level and triggers a repaint. */
    void SetGlowIntensity(qreal intensity);

    /** @brief Gets the current electronic shutdown effect progress. */
    qreal GetShutdownProgress() const { return shutdown_progress_; }

    /** @brief Sets the electronic shutdown effect progress and updates the ElectronicShutdownEffect. */
    void SetShutdownProgress(qreal progress);

    // --- End Property Getters/Setters ---

    // --- Simple Getters ---
    /** @brief Checks if the message has been marked as read. */
    bool IsRead() const { return is_read_; }
    /** @brief Gets the sender identifier. */
    QString GetMessageSender() const { return sender_; }
    /** @brief Gets the original message content (may include HTML). */
    QString GetContent() const { return content_; }
    /** @brief Gets the message type (Received, Transmitted, System). */
    MessageType GetType() const { return type_; }
    /** @brief Gets the unique message identifier (if provided). */
    QString GetMessageId() const { return message_id_; }
    // --- End Simple Getters ---

    /**
     * @brief Parses HTML content to find an attachment placeholder and adds the corresponding AttachmentPlaceholder widget.
     * Extracts attachment details (ID, MIME type, filename) from the HTML attributes.
     * Removes the old attachment widget if present. Adjusts message size constraints.
     * Connects signals to handle attachment loading and size changes.
     * @param html The HTML string containing the attachment placeholder div.
     */
    void AddAttachment(const QString &html);

    /**
     * @brief Adjusts the size of the message widget to fit its content, especially after an attachment has loaded.
     * Forces layout recalculation and calculates optimal size based on attachment dimensions,
     * respecting screen boundaries. Updates geometry and parent layout.
     */
    void AdjustSizeToContent();

    /**
     * @brief Returns the recommended size for the widget based on its content (text length or attachment size).
     * @return The calculated QSize hint.
     */
    QSize sizeHint() const override;

    /**
     * @brief Starts the fade-in animation (opacity and glow).
     * Also triggers the text reveal animation (CyberTextDisplay) upon completion. Sets focus.
     */
    void FadeIn();

    /**
     * @brief Starts the fade-out animation (opacity and glow).
     * Hides the widget and emits the hidden() signal upon completion.
     */
    void FadeOut();

    /**
     * @brief Updates the maximum height constraint for the scroll area used for long messages.
     * Limits the height to a percentage of the available screen height.
     */
    void UpdateScrollAreaMaxHeight() const;

    /**
     * @brief Starts the electronic shutdown closing animation.
     * Applies the ElectronicShutdownEffect and animates its progress.
     * Hides the widget and emits hidden() upon completion.
     */
    void StartShutdownAnimation();

    /**
     * @brief Shows or hides the navigation buttons (Next, Previous) and the Mark Read button.
     * Updates the layout to position the buttons correctly.
     * @param has_previous True to show the Previous button.
     * @param has_next True to show the Next button.
     */
    void ShowNavigationButtons(bool has_previous, bool has_next) const;

    /** @brief Gets a pointer to the Next navigation button. */
    QPushButton *GetNextButton() const { return next_button_; }
    /** @brief Gets a pointer to the Previous navigation button. */
    QPushButton *GetPrevButton() const { return prev_button_; }

signals:
    /** @brief Emitted when the message is marked as read (MarkAsRead() is called). */
    void messageRead();

    /** @brief Emitted when the message finishes its fade-out or closing animation and becomes hidden. */
    void hidden();

public slots:
    /**
     * @brief Marks the message as read, triggers the appropriate closing animation
     * (Electronic Shutdown or a simpler fade/glitch for long messages), and emits messageRead().
     * Prevents multiple calls.
     */
    void MarkAsRead();

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom cyberpunk message bubble appearance.
     * Renders the clipped background, neon border, glow effect, decorative lines/markers,
     * sender text, and other AR-style elements based on the message type.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Overridden resize event handler. Updates the layout of internal elements (buttons, scroll area).
     * @param event The resize event.
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief Overridden key press event handler. Handles Space/Enter to mark as read, and Left/Right arrows for navigation.
     * @param event The key event.
     */
    void keyPressEvent(QKeyEvent *event) override;

    /**
     * @brief Overridden focus in event handler. Slightly increases glow intensity for visual feedback.
     * @param event The focus event.
     */
    void focusInEvent(QFocusEvent *event) override;

    /**
     * @brief Overridden focus out event handler. Resets glow intensity.
     * @param event The focus event.
     */
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    /**
     * @brief Slot called by animation_timer_ to update subtle background animations (e.g., glow pulsing).
     */
    void UpdateAnimation();

    /**
     * @brief Adjusts the stylesheet of the scroll area (scrollbar colors) based on the message type.
     */
    void AdjustScrollAreaStyle() const;

private:
    /**
     * @brief Static utility function to extract the value of an HTML attribute from a string.
     * @param html The HTML string.
     * @param attribute The name of the attribute to extract (e.g., "data-attachment-id").
     * @return The attribute's value, or an empty string if not found.
     */
    static QString ExtractAttribute(const QString &html, const QString &attribute);

    /**
     * @brief Starts a simpler closing animation (glitch/fade) used specifically for long messages displayed in a scroll area.
     */
    void StartLongMessageClosingAnimation();

    /** @brief Helper to process image attachments (likely superseded by AddAttachment). */
    void ProcessImageAttachment(const QString &html);

    /** @brief Helper to process GIF attachments (likely superseded by AddAttachment). */
    void ProcessGifAttachment(const QString &html);

    /** @brief Helper to process audio attachments (likely superseded by AddAttachment). */
    void ProcessAudioAttachment(const QString &html);

    /** @brief Helper to process video attachments (likely superseded by AddAttachment). */
    void ProcessVideoAttachment(const QString &html);

    /**
     * @brief Cleans the message content by removing HTML tags and placeholders, and decoding HTML entities.
     * Updates the clean_content_ member variable. Applies specific formatting for long text.
     */
    void CleanupContent();

    /**
     * @brief Updates the position of the navigation and mark read buttons based on the widget's current size.
     */
    void UpdateLayout() const;

    // --- Member Variables ---
    QString message_id_; ///< Unique identifier for the message (optional).
    QString content_; ///< Original message content (may contain HTML).
    QString clean_content_; ///< Processed message content (plain text).
    QString sender_; ///< Sender identifier.
    MessageType type_; ///< Message type (Received, Transmitted, System).
    qreal opacity_; ///< Current opacity level (for animation).
    qreal glow_intensity_; ///< Current glow intensity level (for animation).
    qreal shutdown_progress_; ///< Current electronic shutdown progress.
    bool is_read_; ///< Flag indicating if the message has been marked as read.

    // UI Elements
    QVBoxLayout *main_layout_; ///< Main vertical layout.
    QPushButton *next_button_; ///< Button to navigate to the next message.
    QPushButton *prev_button_; ///< Button to navigate to the previous message.
    QTimer *animation_timer_; ///< Timer for subtle background animations.
    QWidget *attachment_widget_ = nullptr; ///< Widget holding the attachment placeholder/viewer.
    QLabel *content_label_ = nullptr; ///< Label for displaying short text content (if no CyberTextDisplay).
    TextDisplayEffect *text_display_ = nullptr; ///< Widget for animated text reveal (short messages).
    QScrollArea *scroll_area_ = nullptr; ///< Scroll area for long messages.
    LongTextDisplayEffect *long_text_display_ = nullptr; ///< Widget for displaying long text within scroll area.

    // Effects
    ElectronicShutdownEffect *shutdown_effect_ = nullptr; ///< Graphics effect for electronic shutdown animation.
};

#endif // STREAM_MESSAGE_H
