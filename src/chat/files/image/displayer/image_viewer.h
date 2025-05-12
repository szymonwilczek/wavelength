#ifndef INLINE_IMAGE_VIEWER_H
#define INLINE_IMAGE_VIEWER_H

#include <QFrame>

class QLabel;
class ImageDecoder;
class TranslationManager;

/**
 * @brief A widget for displaying static images (PNG, JPEG, etc.) within a chat interface.
 *
 * This class uses ImageDecoder (which relies on Qt's image loading) to decode image data.
 * It displays the image within a QLabel, automatically scaling the content to fit the label's size
 * while preserving an aspect ratio (due to QLabel's setScaledContents(true)).
 * It provides signals for when the image is loaded and when its information (size, alpha) is available.
 */
class ImageViewer final : public QFrame {
    Q_OBJECT

public:
    /**
     * @brief Constructs an InlineImageViewer widget.
     * Initializes the UI (QLabel), creates the ImageDecoder instance, connects signals
     * and triggers asynchronous decoding of the image data.
     * @param image_data The raw image data to be displayed.
     * @param parent Optional parent widget.
     */
    explicit ImageViewer(const QByteArray &image_data, QWidget *parent = nullptr);

    /**
     * @brief Destructor. Ensures resources are released by calling ReleaseResources().
     */
    ~ImageViewer() override {
        ReleaseResources();
    }

    /**
     * @brief Releases resources held by the ImageDecoder.
     * Resets the shared pointer to the decoder.
     */
    void ReleaseResources();

    /**
     * @brief Provides a size hint based on the original dimensions of the loaded image.
     * Returns the actual image size once loaded, otherwise falls back to the default QFrame size hint.
     * @return The recommended QSize for the widget (original image size).
     */
    [[nodiscard]] QSize sizeHint() const override;

private slots:
    /**
     * @brief Initiates the image decoding process by calling Decode() on the ImageDecoder.
     * Called asynchronously via QTimer::singleShot after the constructor finishes.
     */
    void LoadImage() const;

    /**
     * @brief Slot connected to the decoder's imageReady signal.
     * Stores the decoded image, sets it as the pixmap for the internal QLabel,
     * updates the widget's geometry, and emits the imageLoaded() signal.
     * Relies on the QLabel's setScaledContents(true) for display scaling.
     * @param image The successfully decoded QImage.
     */
    void HandleImageReady(const QImage &image);

    /**
     * @brief Slot connected to the decoder's error signal.
     * Logs the error and displays an error message in the QLabel.
     * Sets a minimum size to ensure the error message is visible.
     * @param message The error message from the decoder.
     */
    void HandleError(const QString &message);

    /**
     * @brief Slot connected to the decoder's imageInfo signal.
     * Stores the image dimensions and alpha channel information.
     * Emits the imageInfoReady() signal.
     * @param width The width of the image in pixels.
     * @param height The height of the image in pixels.
     * @param has_alpha True if the image has an alpha channel.
     */
    void HandleImageInfo(int width, int height, bool has_alpha);

signals:
    /**
     * @brief Emitted after the image information (dimensions, alpha) has been successfully received from the decoder.
     * @param width The width of the image in pixels.
     * @param height The height of the image in pixels.
     * @param has_alpha True if the image has an alpha channel.
     */
    void imageInfoReady(int width, int height, bool has_alpha);

    /**
     * @brief Emitted after the image has been successfully decoded and set in the QLabel.
     * Can be used by parent widgets to adjust layout once the image is ready for display.
     */
    void imageLoaded();

private:
    /** @brief QLabel used to display the image. ScaledContents is enabled. */
    QLabel *image_label_;
    /** @brief Shared pointer to the ImageDecoder instance responsible for decoding. */
    std::shared_ptr<ImageDecoder> decoder_;
    /** @brief Stores the raw image data passed in the constructor. */
    QByteArray image_data_;
    /** @brief Stores the original, unscaled decoded image. */
    QImage original_image_;

    /** @brief Original width of the image in pixels. Set by HandleImageInfo. */
    int image_width_ = 0;
    /** @brief Original height of the image in pixels. Set by HandleImageInfo. */
    int image_height_ = 0;
    /** @brief Flag indicating if the image has an alpha channel. Set by HandleImageInfo. */
    bool has_alpha_ = false;
    /** @brief Pointer to the translation manager for handling UI translations. */
    TranslationManager *translator_ = nullptr;
};

#endif // INLINE_IMAGE_VIEWER_H
