#ifndef RESIZE_EVENT_FILTER_H
#define RESIZE_EVENT_FILTER_H

#include <QObject>

class BlobAnimation;
class QLabel;

/**
 * @brief An event filter specifically designed to center a QLabel within a BlobAnimation widget whenever the BlobAnimation widget is resized.
 *
 * This filter is installed on the BlobAnimation widget. It intercepts QEvent::Resize events
 * targeted at the BlobAnimation widget and calls WavelengthUtilities::CenterLabel to ensure
 * the associated QLabel remains centered within the animation area.
 */
class ResizeEventFilter final : public QObject {
public:
    /**
     * @brief Constructs a ResizeEventFilter.
     * @param label The QLabel widget that needs to be centered.
     * @param animation The BlobAnimation widget within which the label should be centered, and which will be monitored for resize events. The animation widget is also set as the parent of this event filter object.
     */
    ResizeEventFilter(QLabel *label, BlobAnimation *animation);

protected:
    /**
     * @brief Filters events sent to the watched object (the BlobAnimation widget).
     * Checks if the event is a Resize event for the BlobAnimation widget. If it is,
     * it calls WavelengthUtilities::CenterLabel to recenter the label_.
     * @param watched The object that generated the event (expected to be animation_).
     * @param event The event being processed.
     * @return Always returns false, indicating that the event should be processed further by the watched object after the centering logic is executed.
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    /** @brief Pointer to the QLabel that needs to be centered. */
    QLabel *label_;
    /** @brief Pointer to the BlobAnimation widget being monitored for resize events. */
    BlobAnimation *animation_;
};

#endif //RESIZE_EVENT_FILTER_H
