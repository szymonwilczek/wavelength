#ifndef HANDPRINT_LAYER_H
#define HANDPRINT_LAYER_H

#include "../security_layer.h"
#include <QProgressBar>
#include <QSvgRenderer>
#include <QWidget>
#include <QImage>
#include <QLabel>
#include <QStringList>

/**
 * @brief A security layer simulating handprint authentication.
 *
 * This layer displays a handprint image (loaded from an SVG file) and requires the user
 * to press and hold on the image. A progress bar fills up while the user holds the mouse button.
 * The handprint image visually updates during the "scan" process, changing color from gray
 * to blue as the progress increases. Upon successful completion (progress reaches 100%),
 * the UI elements turn green, and the layer fades out, emitting the layerCompleted() signal.
 */
class HandprintLayer final : public SecurityLayer {
    Q_OBJECT

public:
    /**
     * @brief Constructs a HandprintLayer.
     * Initializes the UI elements (title, handprint image label, instructions, progress bar),
     * sets up the layout, creates the progress timer, loads the list of available handprint SVG files,
     * and initializes the SVG renderer.
     * @param parent Optional parent widget.
     */
    explicit HandprintLayer(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops and deletes the progress timer and the SVG renderer.
     */
    ~HandprintLayer() override;

    /**
     * @brief Initializes the layer for display.
     * Resets the progress bar and styles, loads the handprint image, and ensures the layer is fully opaque.
     */
    void Initialize() override;

    /**
     * @brief Resets the layer to its initial state.
     * Stops the progress timer, resets the progress bar value and style, resets the handprint image style,
     * restores the base handprint image, and ensures the layer is fully opaque.
     */
    void Reset() override;

private slots:
    /**
     * @brief Slot called periodically by handprint_timer_ when the user is holding the mouse button.
     * Increments the progress bar value, updates the visual appearance of the handprint scan,
     * and calls ProcessHandprint(true) when the progress reaches 100%.
     */
    void UpdateProgress();

    /**
     * @brief Handles the completion of the handprint scan.
     * Changes the UI element styles (border, progress bar chunk) to green, updates the handprint image
     * to be fully green, and initiates the fade-out animation after a short delay.
     * @param completed Always true in the current implementation, indicating successful completion.
     */
    void ProcessHandprint(bool completed);

private:
    /**
     * @brief Loads the handprint SVG file from handprint_files_ and renders it into base_handprint_.
     * Handles potential loading errors. Sets the initial pixmap on handprint_image_.
     * Note: Currently only supports one hardcoded handprint file.
     */
    void LoadRandomHandprint();

    /**
     * @brief Filters mouse press and release events on the handprint_image_ label.
     * Starts the handprint_timer_ on left mouse button press and stops it on release.
     * @param obj The object that generated the event.
     * @param event The event being processed.
     * @return True if the event was handled (mouse press/release on the image), false otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * @brief Updates the visual appearance of the handprint image during the scan.
     * Renders the SVG and applies a color overlay: gray for the unscanned part and blue
     * for the scanned part, based on the current progress value.
     * @param progress_value The current progress percentage (0-100).
     */
    void UpdateHandprintScan(int progress_value) const;

    /** @brief Label displaying the handprint image. */
    QLabel *handprint_image_;
    /** @brief Progress bar indicating the scan progress. */
    QProgressBar *handprint_progress_;
    /** @brief Timer controlling the progress update while the mouse button is held. */
    QTimer *handprint_timer_;

    /** @brief Renderer used to load and draw the SVG handprint image. */
    QSvgRenderer *svg_renderer_;
    /** @brief The base image of the handprint (rendered SVG in gray). */
    QImage base_handprint_;
    /** @brief List of paths to available handprint SVG files (currently only one). */
    QStringList handprint_files_;
    /** @brief Path to the currently loaded handprint SVG file. */
    QString current_handprint_;
};

#endif // HANDPRINT_LAYER_H
