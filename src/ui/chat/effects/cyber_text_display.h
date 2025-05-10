#ifndef CYBER_TEXT_DISPLAY_H
#define CYBER_TEXT_DISPLAY_H

#include <QAudioOutput>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPainter>
#include <QFontDatabase>
#include <QMediaPlayer>
#include <QMediaPlaylist>

/**
 * @brief A custom widget that displays text with cyberpunk terminal-style "typing" reveal animation and glitch effects.
 *
 * This widget simulates text being typed out character by character, accompanied by an optional typing sound effect.
 * It also features random visual "glitch" effects and a pulsing cursor. The text is rendered with a monospace font
 * and neon colors against a gradient background with scanlines. It handles HTML removal from the input text and
 * recalculates its required height based on content and width.
 */
class CyberTextDisplay final : public QWidget {
    Q_OBJECT
    /** @brief Property controlling the number of characters currently revealed in the typing animation. Animatable. */
    Q_PROPERTY(int revealedChars READ GetRevealedChars WRITE SetRevealedChars)
    /** @brief Property controlling the intensity of the visual glitch effect (0.0 to 1.0+). Animatable. */
    Q_PROPERTY(qreal glitchIntensity READ GetGlitchIntensity WRITE SetGlitchIntensity)

public:
    /**
     * @brief Enum defining the type of typing sound effect to use.
     */
    enum TypingSoundType {
        kSystemSound, ///< Use the system/alternative typing sound (terminal_typing2.wav).
        kUserSound ///< Use the default user typing sound (terminal_typing1.wav).
    };

    Q_ENUM(TypingSoundType)

    /**
     * @brief Constructs a CyberTextDisplay widget.
     * Initializes the widget with text, sets up timers for text reveal and glitch effects,
     * loads the monospace font, initializes the media player for typing sounds, and calculates initial height.
     * @param text The initial text content (can include basic HTML for formatting, which will be stripped for display).
     * @param sound_type The type of typing sounds to use (defaults to kUserSound).
     * @param parent Optional parent widget.
     */
    explicit CyberTextDisplay(const QString &text, TypingSoundType sound_type = kUserSound, QWidget *parent = nullptr);

    /**
     * @brief Destructor. Stops the media player if it's playing.
     */
    ~CyberTextDisplay() override;

    /**
     * @brief Starts or restarts the text revealing animation from the beginning.
     * If the text has been fully revealed once, it instantly shows the full text.
     * Otherwise, resets the revealed character count and starts the text reveal timer and typing sound.
     */
    void StartReveal();

    /**
     * @brief Sets new text content for the widget.
     * Updates the internal text, removes HTML, recalculates required height, resets the reveal state,
     * and starts the reveal animation for the new text.
     * @param new_text The new text content.
     */
    void SetText(const QString &new_text);

    /**
     * @brief Gets the current number of revealed characters.
     * @return The count of revealed characters.
     */
    int GetRevealedChars() const { return revealed_chars_; }

    /**
     * @brief Sets the number of revealed characters.
     * Updates the internal count, triggers a repaint, and emits fullTextRevealed if the text becomes fully revealed.
     * @param chars The desired number of revealed characters (clamped to text length).
     */
    void SetRevealedChars(int chars);

    /**
     * @brief Gets the current intensity of the glitch effect.
     * @return The glitch intensity value (typically 0.0 to 1.0+).
     */
    qreal GetGlitchIntensity() const { return glitch_intensity_; }

    /**
     * @brief Sets the intensity of the glitch effect.
     * Updates the internal intensity value and triggers a repaint.
     * @param intensity The desired glitch intensity.
     */
    void SetGlitchIntensity(qreal intensity);

    /**
     * @brief Enables or disables the random glitch effect timer.
     * When disabled, it also resets the glitch intensity to 0.
     * @param enabled True to enable the glitch timer, false to disable it.
     */
    void SetGlitchEffectEnabled(bool enabled);

    /**
     * @brief Returns the recommended size for the widget based on the calculated height of the text content.
     * @return The calculated QSize hint.
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Emitted exactly once when the text revealing animation completes for the first time
     *        after StartReveal() or SetText() is called.
     */
    void fullTextRevealed();

protected:
    /**
     * @brief Overridden paint event handler. Draws the widget's appearance.
     * Renders the background gradient, the revealed portion of the text with character-by-character coloring,
     * the pulsing cursor, and the scanline effect. Applies glitch effects if intensity > 0.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Overridden resize event handler. Recalculates the required height based on the new width.
     * @param event The resize event.
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief Overridden hide event handler. Stops the typing sound and timers when the widget is hidden.
     * @param event The hide event.
     */
    void hideEvent(QHideEvent *event) override;

private slots:
    /**
     * @brief Slot connected to the fullTextRevealed signal. Stops the text reveal timer and typing sound.
     */
    void HandleFullTextRevealed() const;

    /**
     * @brief Slot called by the text_timer_. Reveals the next character in the sequence.
     * Adjusts the timer interval for pauses after spaces/newlines. Randomly triggers glitches.
     */
    void RevealNextChar();

    /**
     * @brief Slot called by the glitch_timer_. Randomly triggers a glitch effect or fades the current intensity.
     */
    void RandomGlitch();

    /**
     * @brief Initiates a short, animated glitch effect by animating the glitchIntensity property.
     */
    void TriggerGlitch();

private:
    /**
     * @brief Static utility function to remove basic HTML tags and decode common entities from a string.
     * @param html The input string potentially containing HTML.
     * @return The plain text string.
     */
    static QString RemoveHtml(const QString &html);

    /**
     * @brief Recalculates the required height for the widget based on the plain_text_ content and current width.
     * Performs basic word wrapping and updates the widget's minimum height.
     */
    void RecalculateHeight();

    /** @brief The original text content, potentially including HTML. */
    QString full_text_;
    /** @brief The processed text content with HTML removed and potentially word-wrapped by RecalculateHeight. */
    QString plain_text_;
    /** @brief The number of characters currently visible in the reveal animation. */
    int revealed_chars_;
    /** @brief The current intensity level of the visual glitch effect. */
    qreal glitch_intensity_;
    /** @brief Flag indicating if the text is currently fully revealed. */
    bool is_fully_revealed_;
    /** @brief The monospace font used for rendering. */
    QFont font_;
    /** @brief Timer controlling the character-by-character reveal animation. */
    QTimer *text_timer_;
    /** @brief Timer controlling the random triggering and fading of glitch effects. */
    QTimer *glitch_timer_;
    /** @brief Flag indicating if the text has completed its reveal animation at least once since the last SetText(). */
    bool has_been_fully_revealed_once_;
    /** @brief Media player responsible for playing the typing sound effect. */
    QMediaPlayer *media_player_;
    /** @brief Audio output handler (potentially unused if QMediaPlayer handles output directly). */
    QAudioOutput *audio_output_;
    /** @brief Playlist used to loop the typing sound effect in the media player. */
    QMediaPlaylist *playlist_;
    /** @brief The selected type of typing sound effect (determines the audio file used). */
    TypingSoundType sound_type_;
};

#endif // CYBER_TEXT_DISPLAY_H
