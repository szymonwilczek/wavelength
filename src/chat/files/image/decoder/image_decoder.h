#ifndef IMAGE_DECODER_H
#define IMAGE_DECODER_H

#include <QImage>
#include <QObject>

/**
 * @brief Decodes image data using Qt's built-in image loading capabilities.
 *
 * This class takes raw image data (e.g., PNG, JPEG, etc.) as a QByteArray
 * and uses QImage::loadFromData() to decode it. It operates synchronously
 * within the calling thread and emits signals to report the decoded image,
 * image information, or errors.
 */
class ImageDecoder final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs an ImageDecoder object.
     * @param image_data The raw image data to be decoded.
     * @param parent Optional parent QObject.
     */
    explicit ImageDecoder(const QByteArray &image_data, QObject *parent = nullptr)
        : QObject(parent), image_data_(image_data) {
    }

    /**
     * @brief Destructor.
     * Does not require special resource management like FFmpeg-based decoders.
     */
    ~ImageDecoder() override {
    }

    /**
     * @brief Static method placeholder for resource release.
     * In this Qt-based implementation, there are no specific resources to release globally.
     */
    static void ReleaseResources() {
    }

    /**
     * @brief Attempts to decode the image data stored internally.
     * Uses QImage::loadFromData() to perform the decoding. Emits imageReady() and imageInfo()
     * on success, or error() on failure.
     * @return The decoded QImage on success, or a null QImage on failure.
     */
    QImage Decode();

signals:
    /**
     * @brief Emitted when the image has been successfully decoded.
     * @param image The decoded image as a QImage object. A copy is emitted.
     */
    void imageReady(const QImage &image);

    /**
     * @brief Emitted if an error occurs during image loading or decoding.
     * @param message A description of the error.
     */
    void error(const QString &message);

    /**
     * @brief Emitted after successful decoding, providing basic image information.
     * @param width The width of the decoded image in pixels.
     * @param height The height of the decoded image in pixels.
     * @param has_alpha True if the image format supports an alpha channel, false otherwise.
     */
    void imageInfo(int width, int height, bool has_alpha);

private:
    /** @brief Stores the raw image data provided in the constructor. */
    QByteArray image_data_;
};

#endif // IMAGE_DECODER_H
