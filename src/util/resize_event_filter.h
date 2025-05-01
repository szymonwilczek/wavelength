#ifndef RESIZE_EVENT_FILTER_H
#define RESIZE_EVENT_FILTER_H

#include "wavelength_utilities.h"
#include "../blob/core/blob_animation.h"

class ResizeEventFilter final : public QObject {
public:
    ResizeEventFilter(QLabel *label, BlobAnimation *animation)
        : QObject(animation), label_(label), animation_(animation) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLabel *label_;
    BlobAnimation *animation_;
};



#endif //RESIZE_EVENT_FILTER_H
