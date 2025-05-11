#ifndef VOICE_RECOGNITION_LAYER_H
#define VOICE_RECOGNITION_LAYER_H

#include "../security_layer.h"

class QAudioInput;
class QProgressBar;
class QLabel;
class TranslationManager;

/**
 * @brief A security layer simulating voice recognition verification.
 *
 * This layer captures audio input from the default microphone and displays a real-time
 * audio waveform visualization. It requires the user to speak continuously for a set duration
 * (simulated by a progress bar filling up while speech is detected above a noise threshold).
 * Upon successful completion (progress reaches 100%), the UI elements turn green,
 * and the layer fades out, emitting the layerCompleted() signal.
 */
class VoiceRecognitionLayer final : public SecurityLayer {
    Q_OBJECT

public:
    /**
     * @brief Constructs a VoiceRecognitionLayer.
     * Initializes the UI elements (title, audio visualizer, instructions, progress bar),
     * sets up the layout, creates timers for progress, recognition duration, and audio processing,
     * and initializes audio-related variables.
     * @param parent Optional parent widget.
     */
    explicit VoiceRecognitionLayer(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops audio recording and processing, stops and deletes timers.
     */
    ~VoiceRecognitionLayer() override;

    /**
     * @brief Initializes the layer for display.
     * Resets the layer state, ensures the layer is fully opaque, and starts audio recording
     * after a short delay.
     */
    void Initialize() override;

    /**
     * @brief Resets the layer to its initial state.
     * Stops recording and timers, resets progress, clears audio buffers and visualizer data,
     * resets UI element styles, and ensures the layer is fully opaque.
     */
    void Reset() override;

private slots:
    /**
     * @brief Slot called periodically by audio_process_timer_ to read and process available audio data.
     * Reads data from the audio input device, calculates the current audio level, determines if
     * the user is speaking (using a threshold and hysteresis), updates the visualizer data,
     * and appends audio data to the buffer if speech is detected.
     */
    void ProcessAudioInput();

    /**
     * @brief Slot called periodically by progress_timer_ to update the recognition progress bar.
     * Increments the progress bar value only if speech is currently detected (is_speaking_ is true).
     * Calls FinishRecognition() when the progress reaches 100%.
     */
    void UpdateProgress();

    /**
     * @brief Slot called when the recognition process completes (either by progress reaching 100% or recognition_timer_ timeout).
     * Stops recording and timers, updates UI elements to green to indicate success, renders a final
     * green waveform, and initiates the fade-out animation after a short delay.
     */
    void FinishRecognition();

private:
    /**
     * @brief Starts capturing audio from the default input device.
     * Configures the audio format, initializes QAudioInput, starts the input device,
     * and starts the associated timers (progress, recognition duration, audio processing).
     */
    void StartRecording();

    /**
     * @brief Stops capturing audio and processing.
     * Stops the QAudioInput, deletes the audio input object, stops the audio processing timer,
     * and sets the is_recording_ flag to false.
     */
    void StopRecording();

    /**
     * @brief Updates the audio_visualizer_label_ with a waveform representation of the audio data.
     * Shifts existing visualizer data, adds new amplitude data, and redraws the waveform
     * using QPainter. The color of the waveform changes based on whether speech is detected.
     * Also draws a noise threshold indicator and a status text.
     * @param data The raw audio data chunk to visualize (used to calculate the new amplitude).
     */
    void UpdateAudioVisualizer(const QByteArray &data);

    /**
     * @brief Determines if the given audio level exceeds the noise threshold.
     * @param audio_level The calculated audio level (typically normalized RMS or average absolute value).
     * @return True if the level is above the threshold, false otherwise.
     * @note Actual speech detection logic with hysteresis is implemented in ProcessAudioInput().
     */
    bool IsSpeaking(float audio_level) const;

    /** @brief Label used as a canvas to draw the audio waveform visualization. */
    QLabel *audio_visualizer_label_;
    /** @brief Progress bar indicating the voice recognition progress. */
    QProgressBar *recognition_progress_;
    /** @brief Timer controlling the progress bar update based on speech detection. */
    QTimer *progress_timer_;
    /** @brief Timer defining the maximum duration for the recognition attempt. */
    QTimer *recognition_timer_;
    /** @brief Timer controlling how often audio input is processed. */
    QTimer *audio_process_timer_;

    /** @brief Object managing audio input capture. */
    QAudioInput *audio_input_;
    /** @brief I/O device providing access to the raw audio stream from audio_input_. */
    QIODevice *audio_device_;

    /** @brief Buffer storing raw audio data captured while the user is speaking. */
    QByteArray audio_buffer_;
    /** @brief Vector storing normalized amplitude values for rendering the waveform visualizer. */
    QVector<float> visualizer_data_;
    /** @brief Flag indicating if audio is currently being recorded. */
    bool is_recording_;

    /** @brief Threshold level above which audio input is considered potential speech. */
    float noise_threshold_;
    /** @brief Flag indicating if speech is currently detected (based on threshold and hysteresis). */
    bool is_speaking_;
    /** @brief Counter tracking the duration of silence in milliseconds. */
    int silence_counter_;
    /** @brief The calculated audio level from the most recently processed audio chunk. */
    float current_audio_level_;
    /** @brief The translation manager for handling UI translations. */
    TranslationManager *translator_;
};

#endif // VOICE_RECOGNITION_LAYER_H
