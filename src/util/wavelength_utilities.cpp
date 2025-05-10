#include "wavelength_utilities.h"

#include <QString>

#include "../blob/core/blob_animation.h"

double WavelengthUtilities::NormalizeFrequency(const double frequency) {
    const QString formatted = QString::number(frequency, 'f', 1);
    bool ok;
    const double result = formatted.toDouble(&ok);

    if (!ok) {
        qDebug() << "[UTIL] Failed to normalize frequency:" << frequency;
        return frequency;
    }

    return result;
}

void WavelengthUtilities::CenterLabel(QLabel *label, const BlobAnimation *animation) {
    if (label && animation) {
        label->setGeometry(
            (animation->width() - label->sizeHint().width()) / 2,
            (animation->height() - label->sizeHint().height()) / 2,
            label->sizeHint().width(),
            label->sizeHint().height()
        );
        label->raise();
    }
}

void WavelengthUtilities::UpdateTitleLabelStyle(QLabel *label, const QColor &text_color, const QColor &border_color) {
    if (!label) return;
    const QString style = QString(
        "QLabel {"
        "   font-family: 'Blender Pro Heavy';"
        "   letter-spacing: 8px;"
        "   color: %1;"
        "   background-color: transparent;"
        "   border: 2px solid %2;"
        "   border-radius: 8px;"
        "   padding: 10px 20px;"
        "   text-transform: uppercase;"
        "}"
    ).arg(text_color.name(QColor::HexRgb), border_color.name(QColor::HexRgb));
    label->setStyleSheet(style);
    qDebug() << "[UTIL] Updated titleLabel style:" << style;
}
