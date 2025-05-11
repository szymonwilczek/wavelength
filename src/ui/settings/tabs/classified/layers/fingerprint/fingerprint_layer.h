#ifndef FINGERPRINT_LAYER_H
#define FINGERPRINT_LAYER_H

#include "../security_layer.h"

class QSvgRenderer;
class QProgressBar;
class QLabel;

/**
 * @brief A security layer simulating fingerprint authentication.
 *
 * This layer displays a fingerprint image (loaded from an SVG file) and requires the user
 * to press and hold on the image. A progress bar fills up while the user holds the mouse button.
 * The fingerprint image visually updates during the "scan" process, changing color from gray
 * to blue as the progress increases. Upon successful completion (progress reaches 100%),
 * the UI elements turn green, and the layer fades out, emitting the layerCompleted() signal.
 */
class FingerprintLayer final : public SecurityLayer {
    Q_OBJECT

public:
    /**
     * @brief Constructs a FingerprintLayer.
     * Initializes the UI elements (title, fingerprint image label, instructions, progress bar),
     * sets up the layout, creates the progress timer, loads the list of available fingerprint SVG files,
     * and initializes the SVG renderer.
     * @param parent Optional parent widget.
     */
    explicit FingerprintLayer(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops and deletes the progress timer and the SVG renderer.
     */
    ~FingerprintLayer() override;

    /**
     * @brief Initializes the layer for display.
     * Resets the progress bar and styles, loads a random fingerprint image, and ensures the layer is fully opaque.
     */
    void Initialize() override;

    /**
     * @brief Resets the layer to its initial state.
     * Stops the progress timer, resets the progress bar value and style, resets the fingerprint image style,
     * restores the base fingerprint image, and ensures the layer is fully opaque.
     */
    void Reset() override;

private slots:
    /**
     * @brief Slot called periodically by fingerprint_timer_ when the user is holding the mouse button.
     * Increments the progress bar value, updates the visual appearance of the fingerprint scan,
     * and calls ProcessFingerprint(true) when the progress reaches 100%.
     */
    void UpdateProgress();

    /**
     * @brief Handles the completion of the fingerprint scan.
     * Changes the UI element styles (border, progress bar chunk) to green, updates the fingerprint image
     * to be fully green, and initiates the fade-out animation after a short delay.
     * @param completed Always true in the current implementation, indicating successful completion.
     */
    void ProcessFingerprint(bool completed);

private:
    /**
     * @brief Loads a random fingerprint SVG file from fingerprint_files_ and renders it into base_fingerprint_.
     * Handles potential loading errors. Sets the initial pixmap on fingerprint_image_.
     */
    void LoadRandomFingerprint();

    /**
     * @brief Filters mouse press and release events on the fingerprint_image_ label.
     * Starts the fingerprint_timer_ on left mouse button press and stops it on release.
     * @param obj The object that generated the event.
     * @param event The event being processed.
     * @return True if the event was handled (mouse press/release on the image), false otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * @brief Updates the visual appearance of the fingerprint image during the scan.
     * Renders the SVG and applies a color overlay: gray for the unscanned part and blue
     * for the scanned part, based on the current progress value.
     * @param progress_value The current progress percentage (0-100).
     */
    void UpdateFingerprintScan(int progress_value) const;

    /** @brief Label displaying the fingerprint image. */
    QLabel *fingerprint_image_;
    /** @brief Progress bar indicating the scan progress. */
    QProgressBar *fingerprint_progress_;
    /** @brief Timer controlling the progress update while the mouse button is held. */
    QTimer *fingerprint_timer_;
    /** @brief Renderer used to load and draw SVG fingerprint images. */
    QSvgRenderer *svg_renderer_;
    /** @brief The base image of the fingerprint (rendered SVG in gray). */
    QImage base_fingerprint_;
    /** @brief List of paths to available fingerprint SVG files. */
    QStringList fingerprint_files_;
    /** @brief Path to the currently loaded fingerprint SVG file. */
    QString current_fingerprint_;
};

#endif // FINGERPRINT_LAYER_H
