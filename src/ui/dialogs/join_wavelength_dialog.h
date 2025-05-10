#ifndef JOIN_WAVELENGTH_DIALOG_H
#define JOIN_WAVELENGTH_DIALOG_H

#include <QTimer>

#include "animated_dialog.h"

class CyberButton;
class QLabel;
class QComboBox;
class CyberLineEdit;
class TranslationManager;

/**
 * @brief A dialog window for joining an existing Wavelength frequency.
 *
 * This class provides a user interface for entering a frequency and an optional password
 * to join a Wavelength session. It inherits from AnimatedDialog to provide a
 * "digital materialization" animation effect on show/close, featuring a moving scanline.
 * It includes input validation and interacts with WavelengthJoiner to handle the joining process,
 * displaying status messages for success, authentication failure, or connection errors.
 */
class JoinWavelengthDialog final : public AnimatedDialog {
    Q_OBJECT
    /** @brief Property controlling the opacity of the horizontal scanlines effect (0.0 to 1.0). Animatable. */
    Q_PROPERTY(double scanlineOpacity READ GetScanlineOpacity WRITE SetScanlineOpacity)
    /** @brief Property controlling the progress of the main vertical scanline reveal animation (0.0 to 1.0). Animatable. */
    Q_PROPERTY(double digitalizationProgress READ GetDigitalizationProgress WRITE SetDigitalizationProgress)
    /** @brief Property controlling the visibility/intensity of the corner highlight markers (0.0 to 1.0). Animatable. */
    Q_PROPERTY(double cornerGlowProgress READ GetCornerGlowProgress WRITE SetCornerGlowProgress)

public:
    /**
     * @brief Constructs a JoinWavelengthDialog.
     * Sets up the UI elements (labels, input fields, buttons), connects signals and slots
     * for input validation and joining logic, configures the animation type and duration,
     * and initializes the refresh timer for the scanline animation.
     * @param parent Optional parent widget.
     */
    explicit JoinWavelengthDialog(QWidget *parent = nullptr);

    /**
     * @brief Destructor. Stops and deletes the refresh timer.
     */
    ~JoinWavelengthDialog() override;

    /** @brief Gets the current digitalization progress (vertical scanline position). */
    double GetDigitalizationProgress() const { return digitalization_progress_; }

    /** @brief Sets the digitalization progress and triggers a repaint. Starts the refresh timer if needed. */
    void SetDigitalizationProgress(double progress);

    /** @brief Gets the current corner glow progress. */
    double GetCornerGlowProgress() const { return corner_glow_progress_; }

    /** @brief Sets the corner glow progress and triggers a repaint. */
    void SetCornerGlowProgress(double progress);

    /** @brief Gets the current opacity of the horizontal scanlines. */
    double GetScanlineOpacity() const { return scanline_opacity_; }

    /** @brief Sets the opacity of the horizontal scanlines and triggers a repaint. */
    void SetScanlineOpacity(double opacity);

    /**
     * @brief Overridden paint event handler. Draws the custom dialog appearance.
     * Renders the background gradient, clipped border, the main vertical scanline reveal effect,
     * corner highlight markers, and the horizontal scanline overlay effect based on the
     * corresponding progress/opacity properties.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Gets the frequency entered by the user from the input field.
     * Note: This returns the raw text; unit conversion might be needed.
     * @return The frequency string.
     */
    QString GetFrequency() const;

    /**
     * @brief Gets the password entered by the user from the input field.
     * @return The password string.
     */
    QString GetPassword() const;

    /** @brief Gets a pointer to the internal refresh timer used for scanline animation updates. */
    QTimer *GetRefreshTimer() const { return refresh_timer_; }
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

private slots:
    /**
     * @brief Validates the frequency input field.
     * Enables the "Join" button only if the frequency field is not empty. Hides the status label.
     */
    void ValidateInput() const;

    /**
     * @brief Attempts to join the wavelength using the entered frequency and password.
     * Shows a wait cursor, calls WavelengthJoiner::JoinWavelength, and handles immediate
     * failure responses by showing an error message and triggering a glitch animation.
     * Uses a static flag to prevent concurrent join attempts.
     */
    void TryJoin();

    /**
     * @brief Slot triggered by WavelengthJoiner when authentication fails (incorrect password).
     * Displays an authentication failure message in the status label and triggers a glitch animation.
     */
    void OnAuthFailed();

    /**
     * @brief Slot triggered by WavelengthJoiner when a connection error occurs.
     * Displays an appropriate error message (e.g., "Password required", "Wavelength unavailable")
     * in the status label and triggers a glitch animation.
     * @param error_message The error message received from the joiner.
     */
    void OnConnectionError(const QString &error_message);

private:
    /**
     * @brief Initializes or reinitializes the QPixmap buffer used for rendering the vertical scanline effect.
     * Called when needed by paintEvent, typically on the first paint or after a resize.
     * Creates a pixmap with a vertical gradient representing the scanline glow.
     */
    void InitRenderBuffers();

    /** @brief Flag indicating if the show animation has started (used to control scanline drawing). */
    bool animation_started_ = false;
    /** @brief Input field for the wavelength frequency. */
    CyberLineEdit *frequency_edit_;
    /** @brief Dropdown for selecting the frequency unit (Hz, kHz, MHz). */
    QComboBox *frequency_unit_combo_;
    /** @brief Input field for the wavelength password (optional). */
    CyberLineEdit *password_edit_;
    /** @brief Label to display status or error messages during the join process. */
    QLabel *status_label_;
    /** @brief Button to initiate the join attempt. */
    CyberButton *join_button_;
    /** @brief Button to cancel the dialog. */
    CyberButton *cancel_button_;
    /** @brief Timer used to trigger repaints for the vertical scanline animation. */
    QTimer *refresh_timer_;

    /** @brief Size parameter for shadow effects (potentially unused). */
    const int shadow_size_;
    /** @brief Current opacity for the horizontal scanline effect. */
    double scanline_opacity_;
    /** @brief Current progress for the vertical scanline reveal animation. */
    double digitalization_progress_ = 0.0;
    /** @brief Current progress for the corner highlight animation. */
    double corner_glow_progress_ = 0.0;
    /** @brief Buffer holding the pre-rendered vertical scanline gradient. */
    QPixmap scanline_buffer_;
    /** @brief Flag indicating if the render buffers (scanline_buffer_) have been initialized. */
    bool buffers_initialized_ = false;
    /** @brief Stores the last Y position of the scanline to optimize repaint regions. */
    int last_scanline_y_ = -1;
    /** @brief Stores the previous height to detect resize events for buffer reinitialization. */
    int previous_height_ = 0;
    /** @brief Pointer to the translation manager for handling UI translations. */
    TranslationManager *translator_ = nullptr;
};

#endif // JOIN_WAVELENGTH_DIALOG_H
