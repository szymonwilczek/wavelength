#ifndef COMMUNICATION_STREAM_H
#define COMMUNICATION_STREAM_H

#include <qcoreapplication.h>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include "stream_message.h"
#include "../labels/user_info_label.h"

class WavelengthConfig;
/**
 * @brief Structure holding visual properties generated for a user ID.
 * Currently only holds the generated color.
 */
struct UserVisuals {
    /** @brief A unique color generated based on the user ID hash. */
    QColor color;
};

/**
 * @brief A custom OpenGL widget displaying an animated communication stream visualization.
 *
 * This widget renders a dynamic sine wave with cyberpunk aesthetics, including grid lines,
 * glitch effects, and scanlines, using OpenGL shaders. It can display incoming messages
 * (StreamMessage) overlaid on the animation. The wave's amplitude reacts to audio input
 * (via SetAudioAmplitude) and changes appearance based on its state (Idle, Receiving, Displaying).
 * It also shows the name of the stream and the ID of the currently transmitting user.
 * Message navigation (next/previous) and closing are handled via keyboard input (arrows, Enter).
 */
class CommunicationStream final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    /** @brief Property controlling the wave's vertical amplitude. Animatable. */
    Q_PROPERTY(qreal waveAmplitude READ GetWaveAmplitude WRITE SetWaveAmplitude)
    /** @brief Property controlling the wave's horizontal frequency. Animatable. */
    Q_PROPERTY(qreal waveFrequency READ GetWaveFrequency WRITE SetWaveFrequency)
    /** @brief Property controlling the wave's horizontal scrolling speed. Animatable. */
    Q_PROPERTY(qreal waveSpeed READ GetWaveSpeed WRITE SetWaveSpeed)
    /** @brief Property controlling the intensity of visual glitch effects. Animatable. */
    Q_PROPERTY(qreal glitchIntensity READ GetGlitchIntensity WRITE SetGlitchIntensity)
    /** @brief Property controlling the thickness of the rendered wave line. Animatable. */
    Q_PROPERTY(qreal waveThickness READ GetWaveThickness WRITE SetWaveThickness)

public:
    /**
     * @brief Enum defining the different visual states of the communication stream.
     */
    enum StreamState {
        kIdle, ///< Default state: Simple wave with occasional random glitches.
        kReceiving, ///< Active animation during message reception (before display).
        kDisplaying ///< State when a message is actively displayed.
    };

    Q_ENUM(StreamState)

    /**
     * @brief Constructs a CommunicationStream widget.
     * Initializes OpenGL format, timers for animation and glitches, labels for stream name
     * and transmitting user, and sets default animation parameters.
     * @param parent Optional parent widget.
     */
    explicit CommunicationStream(QWidget *parent = nullptr);

    /**
     * @brief Destructor. Cleans up OpenGL resources (VAO, VBO, shader program).
     */
    ~CommunicationStream() override;

    /**
     * @brief Adds a new message with attachment content to the stream.
     * Initiates the receiving animation. The message is displayed after the animation completes
     * if no other message is currently shown. Connects signals for navigation and closing.
     * @param content The main content or description of the attachment.
     * @param sender The sender's identifier.
     * @param type The type of the message (e.g., Attachment).
     * @param message_id Optional unique ID, used for progress messages.
     * @return Pointer to the created StreamMessage widget.
     */
    StreamMessage *AddMessageWithAttachment(const QString &content, const QString &sender,
                                            StreamMessage::MessageType type, const QString &message_id = QString());

    /** @brief Gets the current wave amplitude. */
    qreal GetWaveAmplitude() const { return wave_amplitude_; }

    /** @brief Sets the current wave amplitude and triggers an update. */
    void SetWaveAmplitude(qreal amplitude);

    /** @brief Gets the current wave frequency. */
    qreal GetWaveFrequency() const { return wave_frequency_; }

    /** @brief Sets the current wave frequency and triggers an update. */
    void SetWaveFrequency(qreal frequency);

    /** @brief Gets the current wave speed. */
    qreal GetWaveSpeed() const { return wave_speed_; }

    /** @brief Sets the current wave speed and triggers an update. */
    void SetWaveSpeed(qreal speed);

    /** @brief Gets the current glitch effect intensity. */
    qreal GetGlitchIntensity() const { return glitch_intensity_; }

    /** @brief Sets the current glitch effect intensity and triggers an update. */
    void SetGlitchIntensity(qreal intensity);

    /** @brief Gets the current wave thickness. */
    qreal GetWaveThickness() const { return wave_thickness_; }

    /** @brief Sets the current wave thickness and triggers an update. */
    void SetWaveThickness(qreal thickness);

    /**
     * @brief Sets the text displayed in the stream name label at the top center.
     * @param name The new name for the stream.
     */
    void SetStreamName(const QString &name) const;

    /**
     * @brief Adds a new standard message to the stream.
     * Initiates the receiving animation. The message is displayed after the animation completes
     * if no other message is currently shown. Connects signals for navigation and closing.
     * @param content The message content.
     * @param sender The sender's identifier.
     * @param type The type of the message (e.g., Text, System).
     * @param message_id Optional unique ID, used for progress messages.
     * @return Pointer to the created StreamMessage widget.
     */
    StreamMessage *AddMessage(const QString &content, const QString &sender, StreamMessage::MessageType type,
                              const QString &message_id = QString());

    /**
     * @brief Removes all messages currently managed by the stream.
     * Hides and schedules deletion for all message widgets and resets the stream state.
     */
    void ClearMessages();

public slots:
    /**
     * @brief Displays the identifier of the user currently transmitting audio.
     * Generates a unique color for the user and shows their ID (potentially shortened)
     * in a label at the top right.
     * @param user_id The identifier of the transmitting user.
     */
    void SetTransmittingUser(const QString &user_id) const;

    /**
     * @brief Hides the transmitting user label.
     */
    void ClearTransmittingUser() const;

    /**
     * @brief Sets the target wave amplitude based on incoming audio level.
     * The actual wave amplitude smoothly interpolates towards this target value.
     * @param amplitude The audio amplitude level (typically 0.0 to 1.0).
     */
    void SetAudioAmplitude(qreal amplitude);

protected:
    /**
     * @brief Initializes OpenGL resources (shaders, VAO, VBO). Called once before the first paintGL.
     */
    void initializeGL() override;

    /**
     * @brief Handles resizing of the OpenGL viewport and repositions overlay widgets.
     * @param w New width.
     * @param h New height.
     */
    void resizeGL(int w, int h) override;

    /**
     * @brief Renders the OpenGL scene (animated wave, grid, glitches). Called on update().
     */
    void paintGL() override;

    /**
     * @brief Handles key presses for message navigation (Left/Right arrows) and closing (Enter).
     * @param event The key event.
     */
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    /**
     * @brief Slot called by animation_timer_ to update animation time and parameters.
     * Advances time offset, interpolates wave amplitude towards target, fades glitch intensity,
     * and schedules a repaint.
     */
    void UpdateAnimation();

    /**
     * @brief Slot called by glitch_timer_ to trigger random glitch effects during the Idle state.
     */
    void TriggerRandomGlitch();

    /**
     * @brief Initiates the "Receiving" state animation (wave becomes more active).
     * Animates amplitude, frequency, speed, thickness, and glitch intensity.
     * Transitions to Displaying state upon completion.
     */
    void StartReceivingAnimation();

    /**
     * @brief Initiates the animation returning the wave to the "Idle" state.
     * Animates amplitude, frequency, speed, and thickness back to base values.
     */
    void ReturnToIdleAnimation();

    /**
     * @brief Starts a short, intense glitch animation.
     * @param intensity The peak intensity of the glitch.
     */
    void StartGlitchAnimation(qreal intensity);

    /**
     * @brief Displays the message at the specified index in the messages_ list.
     * Hides the previously displayed message, updates the current index, positions and shows
     * the new message, connects signals, updates navigation buttons, and sets focus.
     * @param index The index of the message to display.
     */
    void ShowMessageAtIndex(int index);

    /**
     * @brief Shows the next message in the list, if available.
     */
    void ShowNextMessage();

    /**
     * @brief Shows the previous message in the list, if available.
     */
    void ShowPreviousMessage();

    /**
     * @brief Slot triggered when the user requests to close the current message (e.g., clicks "Read" or presses Enter).
     * Initiates the process of closing the current message, which might trigger clearing all messages if Enter was pressed.
     */
    void OnMessageRead();

    /**
     * @brief Slot triggered when a StreamMessage widget finishes its hiding animation (or is hidden).
     * Handles logic for removing the message from the list, deleting the widget,
     * showing the next message, or returning to the Idle state. Differentiates between
     * closing a single message, removing a progress message, and clearing all messages.
     */
    void HandleMessageHidden();

    /**
     * @brief Slot called when a configuration setting changes in WavelengthConfig.
     * Updates the stream color if the relevant key ("stream_color" or "all") is provided.
     * @param key The key of the changed setting.
     */
    void UpdateStreamColor(const QString &key);

private:
    /**
     * @brief Generates visual properties (currently color) based on a user ID hash.
     * @param user_id The user identifier string.
     * @return A UserVisuals struct containing the generated properties.
     */
    static UserVisuals GenerateUserVisuals(const QString &user_id);

    /**
     * @brief Connects the necessary signals (navigation, read, hidden) from a StreamMessage widget to this CommunicationStream.
     * Uses Qt::UniqueConnection to prevent duplicate connections.
     * @param message Pointer to the StreamMessage widget.
     */
    void ConnectSignalsForMessage(const StreamMessage *message);

    /**
     * @brief Disconnects all signals originating from the given StreamMessage widget that are connected to this CommunicationStream.
     * @param message Pointer to the StreamMessage widget.
     */
    void DisconnectSignalsForMessage(const StreamMessage *message) const;

    /**
     * @brief Updates the visibility of the "Next" and "Previous" buttons on the currently displayed message
     * based on its position in the messages_ list.
     */
    void UpdateNavigationButtonsForCurrentMessage();

    /**
     * @brief Temporarily reduces the background animation frame rate during message transitions for smoother UI performance.
     */
    void OptimizeForMessageTransition() const;

    /**
     * @brief Updates the position of the currently displayed message to keep it centered.
     */
    void UpdateMessagePosition();


    // Wave Animation Parameters
    const qreal base_wave_amplitude_; ///< Base amplitude in Idle state.
    const qreal amplitude_scale_; ///< Multiplier for audio amplitude effect.
    qreal wave_amplitude_; ///< Current animated wave amplitude.
    qreal target_wave_amplitude_; ///< Target amplitude for smooth interpolation.
    qreal wave_frequency_; ///< Current animated wave frequency.
    qreal wave_speed_; ///< Current animated wave speed.
    qreal glitch_intensity_; ///< Current animated glitch intensity.
    qreal wave_thickness_; ///< Current animated wave thickness.

    // State and Message Management
    StreamState state_; ///< Current visual state of the stream.
    QList<StreamMessage *> messages_; ///< List of message widgets managed by the stream.
    int current_message_index_; ///< Index of the currently displayed message in messages_.
    bool is_clearing_all_messages_ = false; ///< Flag indicating if a full clear operation is in progress.

    // UI Elements
    QLabel *stream_name_label_; ///< Label displaying the stream name.
    UserInfoLabel *transmitting_user_label_; ///< Label displaying the transmitting user ID and color indicator.

    // Internal State
    bool initialized_; ///< Flag indicating if OpenGL resources are initialized.
    qreal time_offset_; ///< Time variable for shader animations.

    // Timers
    QTimer *animation_timer_; ///< Timer driving the main wave animation updates.
    QTimer *glitch_timer_; ///< Timer triggering random glitches in Idle state.

    // OpenGL Resources
    QOpenGLShaderProgram *shader_program_; ///< Compiled shader program for rendering the wave.
    QOpenGLVertexArrayObject vao_; ///< Vertex Array Object.
    QOpenGLBuffer vertex_buffer_; ///< Vertex Buffer Object holding the fullscreen quad vertices.

    // Regedit Config
    WavelengthConfig *config_; ///< Pointer to the RegeditConfig object for accessing configuration settings.
    QColor stream_color_; ///< Color used for rendering the stream color chosen from the registry.
};

#endif // COMMUNICATION_STREAM_H
