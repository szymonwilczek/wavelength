#ifndef WAVELENGTH_CHAT_VIEW_H
#define WAVELENGTH_CHAT_VIEW_H

#include <QAudioInput>
#include <QLineEdit>
#include <QSoundEffect>
#include <QAudioFormat>
#include <QIODevice>

#include "../../ui/chat/wavelength_stream_display.h"
#include "../../session/wavelength_session_coordinator.h"

/**
 * @brief The main view for displaying and interacting with a specific Wavelength chat session.
 *
 * This widget provides the user interface for a single chat, including:
 * - Displaying incoming and outgoing messages (text and file transfers).
 * - An input field for typing messages.
 * - Buttons for sending messages, attaching files, and aborting the connection.
 * - A Push-to-Talk (PTT) button for voice communication.
 * - Visual indicators for connection status, current frequency/name, and who is transmitting audio.
 * - Cyberpunk-themed styling with custom borders, scanlines, and audio visualization.
 * It interacts with WavelengthSessionCoordinator and WavelengthMessageService for session management,
 * message handling, and PTT functionality.
 */
class WavelengthChatView final : public QWidget {
    Q_OBJECT
    /** @brief Property controlling the opacity of the animated scanline effect (0.0 to 1.0). Animatable. */
    Q_PROPERTY(double scanlineOpacity READ GetScanlineOpacity WRITE SetScanlineOpacity)

    /**
     * @brief Enum representing the possible states of the Push-to-Talk (PTT) functionality.
     */
    enum PttState {
        Idle, ///< Not requesting, transmitting, or receiving PTT.
        Requesting, ///< PTT button pressed, waiting for grant from the server.
        Transmitting, ///< PTT granted, microphone is active, sending audio data.
        Receiving ///< Receiving audio data from another user.
    };

public:
    /**
     * @brief Constructs a WavelengthChatView.
     * Initializes the UI elements (labels, message area, input fields, buttons),
     * sets up layouts, initializes audio components (input/output, format, sounds),
     * connects signals/slots for user interaction and backend events, and starts timers
     * for time display and random visual effects.
     * @param parent Optional parent widget.
     */
    explicit WavelengthChatView(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops audio input/output and cleans up audio resources.
     */
    ~WavelengthChatView() override;

    /**
     * @brief Gets the current opacity of the scanline effect.
     * @return The scanline opacity value (0.0 to 1.0).
     */
    [[nodiscard]] double GetScanlineOpacity() const { return scanline_opacity_; }

    /**
     * @brief Sets the opacity of the scanline effect.
     * Triggers a repaint of the widget.
     * @param opacity The desired opacity (0.0 to 1.0).
     */
    void SetScanlineOpacity(double opacity);

    /**
     * @brief Configures the view for a specific Wavelength.
     * Sets the header title with the frequency and optional name, clears the message area,
     * displays a connection message, resets PTT state, enables controls, triggers a connection
     * visual effect, and makes the view visible.
     * @param frequency The frequency of the Wavelength.
     * @param name Optional name associated with the Wavelength.
     */
    void SetWavelength(const QString &frequency, const QString &name = QString());

    /**
     * @brief Handles displaying a received message for the current Wavelength.
     * Adds the message to the message_area_ if the frequency matches the current one.
     * Triggers a visual activity effect.
     * @param frequency The frequency the message was received on.
     * @param message The content of the message.
     */
    void OnMessageReceived(const QString &frequency, const QString &message);

    /**
     * @brief Handles displaying a message sent by the local user on the current Wavelength.
     * Adds the message to the message_area_ if the frequency matches the current one.
     * @param frequency The frequency the message was sent on.
     * @param message The content of the message.
     */
    void OnMessageSent(const QString &frequency, const QString &message) const;

    /**
     * @brief Handles the event when the host closes the current Wavelength.
     * Updates the status indicator, displays a system message, and schedules the view
     * to be cleared, and the wavelengthAborted() signal to be emitted after a delay.
     * Does nothing if the frequency doesn't match or if the local user initiated the closure.
     * @param frequency The frequency of the closed Wavelength.
     */
    void OnWavelengthClosed(const QString &frequency);

    /**
     * @brief Clears the view and resets its state.
     * Resets the current frequency, clears the message area, header, and input field,
     * and hides the view.
     */
    void Clear();

public slots:
    /**
     * @brief Opens a file dialog to allow the user to select a file for sending.
     * If a file is selected, initiates the file sending process via WavelengthMessageService
     * and displays an initial progress message in the message area.
     */
    void AttachFile();

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom cyberpunk border, AR markers, and scanline effect.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private slots:
    /**
     * @brief Slot called when the PTT button is pressed.
     * Plays the PTT activation sound, sets the state to Requesting, updates the button appearance,
     * and sends a PTT request to the server via WavelengthMessageService.
     */
    void OnPttButtonPressed();

    /**
     * @brief Slot called when the PTT button is released.
     * Plays the PTT deactivation sound. If transmitting, stops audio input and sends a PTT release
     * message. Resets the PTT state to Idle and updates the button appearance.
     */
    void OnPttButtonReleased();

    /**
     * @brief Slot called when the server grants the PTT request.
     * If the frequency matches and the state is Requested, sets the state to Transmitting,
     * updates the button appearance, starts audio input, and updates the message area to show
     * the local user is transmitting.
     * @param frequency The frequency for which PTT was granted.
     */
    void OnPttGranted(const QString &frequency);

    /**
     * @brief Slot called when the server denies the PTT request.
     * If the frequency matches and the state is Requested, displays a system message with the reason,
     * resets the PTT state to Idle, and updates the button appearance.
     * @param frequency The frequency for which PTT was denied.
     * @param reason The reason for denial provided by the server.
     */
    void OnPttDenied(const QString &frequency, const QString &reason);

    /**
     * @brief Slot called when another user starts transmitting PTT on the current Wavelength.
     * If the state is Idle, sets the state to Receiving, updates the button appearance (disabling it),
     * starts audio output, and updates the message area to show the remote sender's ID.
     * @param frequency The frequency on which transmission started.
     * @param sender_id The identifier of the user transmitting.
     */
    void OnPttStartReceiving(const QString &frequency, const QString &sender_id);

    /**
     * @brief Slot called when the remote user stops transmitting PTT.
     * If the frequency matches and the state is Receiving, stops audio output, resets the PTT state to Idle,
     * updates the button appearance, clears the transmitting user display in the message area, and resets
     * the audio amplitude visualization.
     * @param frequency The frequency on which transmission stopped.
     */
    void OnPttStopReceiving(const QString &frequency);

    /**
     * @brief Slot called when binary audio data is received from the server.
     * If the frequency matches and the state is Received, writes the data to the audio output device,
     * calculates the amplitude of the received audio, and updates the audio visualization in the message area.
     * @param frequency The frequency the audio data belongs to.
     * @param audio_data The raw audio data chunk.
     */
    void OnAudioDataReceived(const QString &frequency, const QByteArray &audio_data) const;

    /**
     * @brief Slot called when the audio input device has data ready to be read.
     * Reads all available audio data, sends it to the server via WavelengthMessageService,
     * calculates the amplitude, and updates the local audio visualization in the message area.
     */
    void OnReadyReadInput() const;

    /**
     * @brief Slot to update a message in the message area, typically used for file transfer progress.
     * Finds the message by its ID and updates its content.
     * @param message_id The unique identifier of the message to update.
     * @param message The new content for the message.
     */
    void UpdateProgressMessage(const QString &message_id, const QString &message) const;

    /**
     * @brief Sends the text message currently in the input field.
     * Clears the input field and sends the message via WavelengthMessageService.
     */
    void SendMessage() const;

    /**
     * @brief Initiates the process of leaving or closing the current Wavelength.
     * Updates the status indicator, calls the appropriate method on WavelengthSessionCoordinator
     * (CloseWavelength if hosted, LeaveWavelength otherwise), emits wavelengthAborted(), and schedules
     * the view to be cleared after a short delay.
     */
    void AbortWavelength();

    /**
     * @brief Triggers a random, brief increase in the scanline effect opacity.
     */
    void TriggerVisualEffect();

    /**
     * @brief Triggers a more pronounced scanline effect animation when connecting to a Wavelength.
     */
    void TriggerConnectionEffect();

    /**
     * @brief Triggers a subtle scanline effect animation when there is activity (e.g., a message received).
     */
    void TriggerActivityEffect();

signals:
    /**
     * @brief Emitted when the user aborts the connection or the Wavelength is closed by the host.
     * Signals the parent view to switch back.
     */
    void wavelengthAborted();

private:
    /** @brief Label displaying the Wavelength frequency and name. */
    QLabel *header_label_;
    /** @brief Label indicating the connection status ("AKTYWNE POŁĄCZENIE", "ROZŁĄCZANIE", etc.). */
    QLabel *status_indicator_;
    /** @brief Custom widget for displaying the stream of messages and audio visualization. */
    WavelengthStreamDisplay *message_area_;
    /** @brief Input field for typing text messages. */
    QLineEdit *input_field_;
    /** @brief Button to open the file attachment dialog. */
    QPushButton *attach_button_;
    /** @brief Button to send the text message in the input field. */
    QPushButton *send_button_;
    /** @brief Button to disconnect from the current Wavelength. */
    QPushButton *abort_button_;
    /** @brief Button for Push-to-Talk functionality. */
    QPushButton *ptt_button_;

    /** @brief The frequency of the Wavelength currently displayed in this view. "-1.0" if inactive. */
    QString current_frequency_ = "-1.0";
    /** @brief Current opacity level for the scanline effect overlay. */
    double scanline_opacity_;
    /** @brief Flag indicating if the view is in the process of aborting the connection (prevents duplicate actions). */
    bool is_aborting_ = false;
    /** @brief Stores the ID of the user currently transmitting audio (if receiving). */
    QString current_transmitter_id_;

    /** @brief Current state of the Push-to-Talk interaction. */
    PttState ptt_state_;
    /** @brief Object managing audio input from the microphone. */
    QAudioInput *audio_input_;
    /** @brief Object managing audio output to the speakers. */
    QAudioOutput *audio_output_;
    /** @brief I/O device providing access to the raw audio stream from audio_input_. */
    QIODevice *input_device_;
    /** @brief I/O device providing access to the raw audio stream for audio_output_. */
    QIODevice *output_device_;
    /** @brief The audio format used for both input and output (PCM, 16 kHz, 16-bit mono). */
    QAudioFormat audio_format_;
    /** @brief Sound effect played when the PTT button is pressed. */
    QSoundEffect *ptt_on_sound_;
    /** @brief Sound effect played when the PTT button is released. */
    QSoundEffect *ptt_off_sound_;

    TranslationManager *translator_;

    /**
     * @brief Initializes the audio format and creates QAudioInput/QAudioOutput objects.
     * Determines supported formats and sets buffer sizes.
     */
    void InitializeAudio();

    /**
     * @brief Starts capturing audio from the microphone if PTT state is Transmitting.
     * Connects the readyRead signal to OnReadyReadInput().
     */
    void StartAudioInput();

    /**
     * @brief Stops capturing audio from the microphone.
     * Disconnects signals and resets the input device pointer.
     */
    void StopAudioInput();

    /**
     * @brief Starts audio playback if PTT state is Receiving.
     * Prepares the audio output device to receive data.
     */
    void StartAudioOutput();

    /**
     * @brief Stops audio playback.
     * Resets the output device pointer.
     */
    void StopAudioOutput();

    /**
     * @brief Calculates the Root Mean Square (RMS) amplitude of a raw audio buffer.
     * Used for audio visualization. Assumes 16-bit signed integer PCM data.
     * @param buffer The raw audio data.
     * @return The calculated RMS amplitude, normalized to approximately [0.0, 1.0].
     */
    [[nodiscard]] qreal CalculateAmplitude(const QByteArray &buffer) const;

    /**
     * @brief Updates the appearance and enabled state of the PTT button based on the current ptt_state_.
     * Also enables/disables other input controls (text field, send/attach buttons) accordingly.
     */
    void UpdatePttButtonState() const;

    /**
     * @brief Resets the status_indicator_ label to its default "AKTYWNE POŁĄCZENIE" state and style.
     */
    void ResetStatusIndicator() const;
};

#endif // WAVELENGTH_CHAT_VIEW_H
