#include "resize_event_filter.h"

#include <QEvent>

#include "wavelength_utilities.h"
#include "../blob/core/blob_animation.h"

ResizeEventFilter::ResizeEventFilter(QLabel *label, BlobAnimation *animation)
    : QObject(animation), label_(label), animation_(animation) {
}

bool ResizeEventFilter::eventFilter(QObject *watched, QEvent *event) {
    if (watched == animation_ && event->type() == QEvent::Resize) {
        WavelengthUtilities::CenterLabel(label_, animation_);
        return false;
    }
    return false;
}
