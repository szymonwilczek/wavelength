#ifndef RETINA_SCAN_LAYER_H
#define RETINA_SCAN_LAYER_H

#include "../security_layer.h"

class QProgressBar;
class QLabel;

/**
 * @brief A security layer simulating a retina scan process.
 *
 * This layer displays a randomly generated eye image within a circular scanner interface.
 * An animated scanline moves vertically across the eye image while a progress bar fills up.
 * The scan takes a fixed duration. Upon completion, the UI elements turn green,
 * and the layer fades out, emitting the layerCompleted() signal.
 */
class RetinaScanLayer final : public SecurityLayer {
    Q_OBJECT

public:
    /**
     * @brief Constructs a RetinaScanLayer.
     * Initializes the UI elements (title, eye image container, instructions, progress bar),
     * sets up the layout, creates timers for the scan animation and completion, and prepares
     * the base image buffer.
     * @param parent Optional parent widget.
     */
    explicit RetinaScanLayer(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops and deletes the timers.
     */
    ~RetinaScanLayer() override;

    /**
     * @brief Initializes the layer for display.
     * Resets the layer state, generates a new random eye image, ensures the layer is fully opaque,
     * and starts the scan animation after a short delay.
     */
    void Initialize() override;

    /**
     * @brief Resets the layer to its initial state.
     * Stops timers, resets the scanline position and progress bar, resets UI element styles,
     * clears the eye image, and ensures the layer is fully opaque.
     */
    void Reset() override;

private slots:
    /**
     * @brief Slot called periodically by scan_timer_ to update the scan animation.
     * Increments the progress bar, moves the scanline position, redraws the eye image
     * with the updated scanline, and stops the animation when the scanline completes its pass.
     */
    void UpdateScan();

    /**
     * @brief Slot called when the scan completes (either by timer or scanline finishing).
     * Changes the UI element styles (border, progress bar chunk) to green, displays the final
     * eye image without the scanline, and initiates the fade-out animation after a short delay.
     */
    void FinishScan();

private:
    /**
     * @brief Generates a random eye image (iris color, patterns, reflections) and draws it onto base_eye_image_.
     * Sets the initial pixmap on the eye_image_ label.
     */
    void GenerateEyeImage();

    /**
     * @brief Starts the scan animation timers (scan_timer_ and complete_timer_).
     */
    void StartScanAnimation() const;

    /** @brief Label displaying the eye image with the animated scanline. */
    QLabel *eye_image_;
    /** @brief Progress bar indicating the scan progress. */
    QProgressBar *scan_progress_;
    /** @brief Timer controlling the scanline movement and progress bar update. */
    QTimer *scan_timer_;
    /** @brief Timer defining the total duration of the scan process. */
    QTimer *complete_timer_;
    /** @brief Current vertical position of the scanline (0 to 200). */
    int scanline_;

    /** @brief Stores the generated eye image without the scanline overlay. */
    QImage base_eye_image_;
};

#endif // RETINA_SCAN_LAYER_H
