#ifndef WAVELENGTH_DIALOG_H
#define WAVELENGTH_DIALOG_H

#include <QLabel>
#include <QtConcurrent>
#include <QRandomGenerator>

#include "../../session/wavelength_session_coordinator.h"
#include "../../ui/dialogs/animated_dialog.h"
#include "../../ui/buttons/cyber_button.h"
#include "../../ui/checkbox/cyber_checkbox.h"
#include "../input/cyber_line_edit.h"

class TranslationManager;
/**
 * @brief A dialog window for creating a new Wavelength frequency.
 *
 * This class provides a user interface for initiating the creation of a new Wavelength session.
 * It automatically searches for the lowest available frequency via a relay server, allows setting
 * a password, and displays the assigned frequency. It inherits from AnimatedDialog to provide a
 * "digital materialization" animation effect on show/close, featuring a moving scanline.
 * It includes input validation and interacts with the relay server API to find an available frequency.
 */
class WavelengthDialog final : public AnimatedDialog {
    Q_OBJECT
    /** @brief Property controlling the opacity of the horizontal scanlines effect (0.0 to 1.0). Animatable. */
    Q_PROPERTY(double scanlineOpacity READ GetScanlineOpacity WRITE SetScanlineOpacity) // Corrected getter/setter names
    /** @brief Property controlling the progress of the main vertical scanline reveal animation (0.0 to 1.0). Animatable. */
    Q_PROPERTY(double digitalizationProgress READ GetDigitalizationProgress WRITE SetDigitalizationProgress) // Corrected getter/setter names
    /** @brief Property controlling the visibility/intensity of the corner highlight markers (0.0 to 1.0). Animatable. */
    Q_PROPERTY(double cornerGlowProgress READ GetCornerGlowProgress WRITE SetCornerGlowProgress) // Corrected getter/setter names

public:
    /**
     * @brief Constructs a WavelengthDialog.
     * Sets up the UI elements (labels, input fields, buttons, checkbox), connects signals and slots
     * for input validation and frequency searching/generation logic, configures the animation type
     * and duration, and initializes the refresh timer for the scanline animation.
     * Connects the show animation finished signal to start the frequency search.
     * @param parent Optional parent widget.
     */
    explicit WavelengthDialog(QWidget *parent = nullptr);

    /**
     * @brief Destructor. Stops and deletes the refresh timer.
     */
    ~WavelengthDialog() override;

    // --- Animation Property Accessors ---
    /** @brief Gets the current digitalization progress (vertical scanline position). */
    double GetDigitalizationProgress() const { return digitalization_progress_; } // Corrected function name
    /** @brief Sets the digitalization progress and triggers a repaint. Starts the refresh timer if needed. */
    void SetDigitalizationProgress(double progress); // Corrected function name

    /** @brief Gets the current corner glow progress. */
    double GetCornerGlowProgress() const { return corner_glow_progress_; } // Corrected function name
    /** @brief Sets the corner glow progress and triggers a repaint. */
    void SetCornerGlowProgress(double progress); // Corrected function name

    /** @brief Gets the current opacity of the horizontal scanlines. */
    double GetScanlineOpacity() const { return scanline_opacity_; } // Corrected function name
    /** @brief Sets the opacity of the horizontal scanlines and triggers a repaint. */
    void SetScanlineOpacity(double opacity); // Corrected function name
    // --- End Animation Property Accessors ---

    // --- Refresh Timer Control ---
    /** @brief Gets a pointer to the internal refresh timer used for scanline animation updates. */
    QTimer* GetRefreshTimer() const { return refresh_timer_; }
    /** @brief Starts the refresh timer. */
    void StartRefreshTimer() const {
        if (refresh_timer_) {
            refresh_timer_->start();
        }
    }
    /** @brief Stops the refresh timer. */
    void StopRefreshTimer() const {
        if (refresh_timer_) {
            refresh_timer_->stop();
        }
    }
    /** @brief Sets the interval for the refresh timer. */
    void SetRefreshTimerInterval(const int interval) const {
        if (refresh_timer_) {
            refresh_timer_->setInterval(interval);
        }
    }
    // --- End Refresh Timer Control ---

    /**
     * @brief Gets the frequency assigned by the server (or the default if search failed).
     * @return The frequency string (e.g., "130.0").
     */
    QString GetFrequency() const {
        return frequency_;
    }

    /**
     * @brief Checks if the "Password Protected" checkbox is checked.
     * @return True if the checkbox is checked, false otherwise.
     */
    bool IsPasswordProtected() const {
        return password_protected_checkbox_->isChecked();
    }

    /**
     * @brief Gets the password entered by the user.
     * @return The password string.
     */
    QString GetPassword() const {
        return password_edit_->text();
    }

protected: // Changed access specifier to protected for paintEvent
    /**
     * @brief Overridden paint event handler. Draws the custom dialog appearance.
     * Renders the background gradient, clipped border, the main vertical scanline reveal effect,
     * corner highlight markers, and the horizontal scanline overlay effect based on the
     * corresponding progress/opacity properties.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private slots:
    /**
     * @brief Validates the input fields (currently just the password if protection is enabled).
     * Enables the "Create Wavelength" button only if inputs are valid and a frequency has been found.
     * Hides the status label.
     */
    void ValidateInputs() const;

    /**
     * @brief Starts the asynchronous process of finding the lowest available frequency via the relay server.
     * Called automatically after the show animation finishes. Sets up a timeout for the search.
     */
    void StartFrequencySearch();

    /**
     * @brief Attempts to finalize the wavelength generation process.
     * Checks if a password is required and entered. If valid, accepts the dialog.
     * Uses a static flag to prevent concurrent attempts.
     */
    void TryGenerate();

    /**
     * @brief Slot triggered when the frequency search (FindLowestAvailableFrequency) completes.
     * Updates the frequency label with the found frequency (or default), hides the loading indicator,
     * enables the generate button (if inputs are valid), and triggers animations.
     */
    void OnFrequencyFound();

    /**
     * @brief Static utility function to format a frequency value (double) into a display string with units (Hz, kHz, MHz).
     * @param frequency The frequency value in Hz.
     * @return A formatted string (e.g., "1.3 MHz").
     */
    static QString FormatFrequencyText(double frequency);

private:
    /**
     * @brief Static function executed concurrently to query the relay server for the next available frequency.
     * Sends an HTTP GET request to the server's API endpoint, optionally providing a preferred starting frequency.
     * Parses the JSON response to extract the frequency. Returns a default value on error or timeout.
     * @return The available frequency as a string (e.g., "130.0"), or a default value.
     */
    static QString FindLowestAvailableFrequency();

    /**
     * @brief Initializes or reinitializes the QPixmap buffer used for rendering the vertical scanline effect.
     * Called when needed by paintEvent, typically on the first paint or after a resize.
     * Creates a pixmap with a vertical gradient representing the scanline glow.
     */
    void InitRenderBuffers();

    // --- Member Variables ---
    /** @brief Flag indicating if the show animation has started (used to control scanline drawing). */
    bool animation_started_ = false;
    /** @brief Label displaying the assigned frequency. */
    QLabel *frequency_label_;
    /** @brief Label indicating that the frequency search is in progress. */
    QLabel *loading_indicator_;
    /** @brief Checkbox to enable/disable password protection. */
    CyberCheckBox *password_protected_checkbox_;
    /** @brief Input field for the wavelength password. */
    CyberLineEdit *password_edit_;
    /** @brief Label to display status or error messages. */
    QLabel *status_label_;
    /** @brief Button to confirm wavelength creation and close the dialog. */
    CyberButton *generate_button_;
    /** @brief Button to cancel the dialog. */
    CyberButton *cancel_button_;
    /** @brief Watches the QFuture returned by the concurrent frequency search. */
    QFutureWatcher<QString> *frequency_watcher_;
    /** @brief Timer used to trigger repaints for the vertical scanline animation. */
    QTimer *refresh_timer_;
    /** @brief Stores the frequency found by the server or the default value. */
    QString frequency_ = "130.0";
    /** @brief Flag indicating if the frequency search has completed successfully or timed out. */
    bool frequency_found_ = false;
    /** @brief Size parameter for shadow effects (potentially unused). */
    const int shadow_size_;
    /** @brief Current opacity for the horizontal scanline effect. */
    double scanline_opacity_;
    /** @brief Buffer holding the pre-rendered vertical scanline gradient. */
    QPixmap scanline_buffer_;
    /** @brief Flag indicating if the render buffers (scanline_buffer_) have been initialized. */
    bool buffers_initialized_ = false;
    /** @brief Stores the last Y position of the scanline to optimize repaint regions. */
    int last_scanline_y_ = -1;
    /** @brief Stores the previous height to detect resize events for buffer reinitialization. */
    int previous_height_ = 0;
    /** @brief Pointer to the translation manager for handling UI translations. */
    TranslationManager* translator_ = nullptr;
};

#endif // WAVELENGTH_DIALOG_H