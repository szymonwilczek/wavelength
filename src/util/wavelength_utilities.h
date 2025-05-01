#ifndef WAVELENGTH_UTILITIES_H
#define WAVELENGTH_UTILITIES_H
#include <QLabel>


class BlobAnimation;

class WavelengthUtilities {
public:
    static double NormalizeFrequency(double frequency);

    static void CenterLabel(QLabel *label, const BlobAnimation *animation);

    static void UpdateTitleLabelStyle(QLabel* label, const QColor& text_color, const QColor& border_color);
};

#endif // WAVELENGTH_UTILITIES_H