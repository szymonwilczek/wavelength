#ifndef WAVELENGTH_UTILITIES_H
#define WAVELENGTH_UTILITIES_H

#include <cmath>
#include <string>
#include <QString>
#include <QDebug>

class WavelengthUtilities {
public:
    static double normalizeFrequency(double frequency) {
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
};

#endif // WAVELENGTH_UTILITIES_H