#ifndef WAVELENGTH_UTILITIES_H
#define WAVELENGTH_UTILITIES_H
#include <QLabel>

class BlobAnimation;

/**
 * @brief A collection of static utility functions used throughout the Wavelength application.
 *
 * This class provides helper methods for common tasks such as frequency formatting,
 * UI element positioning, and style updates.
 */
class WavelengthUtilities {
public:
    /**
     * @brief Normalizes a frequency value by formatting it to one decimal place.
     * Converts the double to a string with one decimal place and then back to a double.
     * This effectively rounds or truncates the frequency to a standard format.
     * @param frequency The frequency value (in Hz, kHz, or MHz) to normalize.
     * @return The normalized frequency value as a double, or the original value if conversion fails.
     */
    static double NormalizeFrequency(double frequency);

    /**
     * @brief Centers a QLabel widget within the bounds of a BlobAnimation widget.
     * Calculates the appropriate geometry for the label based on its size hint and the
     * animation widget's dimensions, then applies it and raises the label to ensure visibility.
     * @param label Pointer to the QLabel to be centered.
     * @param animation Pointer to the BlobAnimation widget serving as the reference area.
     */
    static void CenterLabel(QLabel *label, const BlobAnimation *animation);

    /**
     * @brief Applies a specific cyberpunk-themed stylesheet to a QLabel, intended for title elements.
     * Sets the font, letter spacing, text color, border color, border radius, padding, and text transformation.
     * @param label Pointer to the QLabel to be styled.
     * @param text_color The desired color for the label's text.
     * @param border_color The desired color for the label's border.
     */
    static void UpdateTitleLabelStyle(QLabel* label, const QColor& text_color, const QColor& border_color);
};

#endif // WAVELENGTH_UTILITIES_H