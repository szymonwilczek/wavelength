#include "wavelength_utilities.h"

#include <QString>
#include <QDebug>

double WavelengthUtilities::NormalizeFrequency(const double frequency) {
    const QString formatted = QString::number(frequency, 'f', 1);
    bool ok;
    const double result = formatted.toDouble(&ok);

    if (!ok) {
        qDebug() << "Failed to normalize frequency:" << frequency;
        return frequency;
    }

    qDebug() << "Normalized frequency:" << frequency << "->" << result;
    return result;
}
