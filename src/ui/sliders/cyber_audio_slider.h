#ifndef CYBER_AUDIO_SLIDER_H
#define CYBER_AUDIO_SLIDER_H

#include <QSlider>

/**
 * @brief A custom QSlider styled with a cyberpunk aesthetic, specifically themed for audio controls.
 *
 * This slider features a dark purple track, a neon-purple progress bar, and a handle
 * with a subtle glow effect that animates on hover. It's designed visually to represent
 * audio volume or similar audio-related settings.
 */
class CyberAudioSlider final : public QSlider {
    Q_OBJECT
    /** @brief Property controlling the intensity of the handle's glow effect (0.0 to 1.0). Animatable. */
    Q_PROPERTY(double glowIntensity READ GetGlowIntensity WRITE SetGlowIntensity)

public:
    /**
     * @brief Constructs a CyberAudioSlider.
     * Initializes the slider with the specified orientation and applies basic styling
     * (transparent background, no border) via stylesheet. Sets the initial glow intensity.
     * @param orientation The orientation of the slider (Horizontal or Vertical).
     * @param parent Optional parent widget.
     */
    explicit CyberAudioSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

    /**
     * @brief Gets the current intensity of the glow effect.
     * @return The glow intensity value (typically 0.0 to 1.0).
     */
    double GetGlowIntensity() const { return glow_intensity_; }

    /**
     * @brief Sets the intensity of the glow effect.
     * Triggers a repaint of the slider.
     * @param intensity The desired glow intensity.
     */
    void SetGlowIntensity(double intensity);

protected:
    /**
     * @brief Overridden paint event handler. Draws the custom slider appearance.
     * Renders the track, the progress bar (filled portion), the handle, and the glow effect
     * using cyberpunk-themed purple/blue colors. Includes decorative lines on the progress bar and handle.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Overridden enter event handler. Animates the glow effect intensity increase on mouse hover.
     * @param event The enter event.
     */
    void enterEvent(QEvent *event) override;

    /**
     * @brief Overridden leave event handler. Animates the glow effect intensity decrease when the mouse leaves.
     * @param event The leave event.
     */
    void leaveEvent(QEvent *event) override;

private:
    /** @brief Current intensity of the glow effect. Modified by animations on hover. */
    double glow_intensity_;
};

#endif //CYBER_AUDIO_SLIDER_H
