#include "resize_event_filter.h"

bool ResizeEventFilter::eventFilter(QObject *watched, QEvent *event) {
    if (watched == animation_ && event->type() == QEvent::Resize) {
        WavelengthUtilities::CenterLabel(label_, animation_);
        return false;
    }
    return false;
}
