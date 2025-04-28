//
// Created by szymo on 21.03.2025.
//

#include "wavelength_utilities.h"

double WavelengthUtilities::normalizeFrequency(double frequency) {
    // Dokładne odwzorowanie algorytmu JavaScript: parseFloat(numFreq.toFixed(1))
    QString formatted = QString::number(frequency, 'f', 1);
    bool ok;
    double result = formatted.toDouble(&ok);

    if (!ok) {
        qDebug() << "Failed to normalize frequency:" << frequency;
        return frequency;
    }

    qDebug() << "Normalized frequency:" << frequency << "->" << result;
    return result;
}
