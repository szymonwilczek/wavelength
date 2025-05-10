#ifndef BLOBEVENTHANDLER_H
#define BLOBEVENTHANDLER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QPointF>
#include <QResizeEvent>
#include <QDebug>

/**
 * @brief Handles and processes window events (move, resize) for the Blob animation.
 *
 * This class filters events for a parent widget's window, specifically focusing on
 * move and resize events. It applies throttling and significance checks to avoid
 * excessive processing, especially during rapid movements or resizing. It emits
 * signals when significant events are detected, allowing other components (like BlobDynamics)
 * to react appropriately. It also manages to enable/disable event processing,
 * useful during transitions or animations.
 */
class BlobEventHandler final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs a BlobEventHandler.
     * Installs an event filter on the parent widget's window.
     * @param parent_widget The widget whose window events will be monitored.
     */
    explicit BlobEventHandler(QWidget *parent_widget);

    /**
     * @brief Destructor.
     * Removes the event filter from the parent widget's window.
     */
    ~BlobEventHandler() override;

    /**
     * @brief Placeholder for processing general events. Currently unused.
     * @return Always returns false (event not handled here).
     */
    static bool ProcessEvent();

    /**
     * @brief Processes a resize event for the parent widget.
     * Applies throttling and checks for significant size changes before emitting signals.
     * @param event The QResizeEvent received by the parent widget.
     * @return True if the event was considered significant and processed, false otherwise.
     */
    bool ProcessResizeEvent(const QResizeEvent *event);

    /**
     * @brief Filters events installed on the parent widget's window.
     * Specifically, intercepts and handles QEvent::Move events via HandleMoveEvent.
     * @param watched The object being watched (should be the parent widget's window).
     * @param event The event being processed.
     * @return True if the event was handled (currently always returns false after processing), false otherwise.
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

    /**
     * @brief Enables event processing.
     * Emits the eventsReEnabled() signal.
     */
    void EnableEvents();

    /**
     * @brief Disables event processing.
     * Filtered events will be ignored until EnableEvents() is called.
     */
    void DisableEvents();

    /**
     * @brief Checks if event processing is currently enabled.
     * @return True if events are enabled, false otherwise.
     */
    bool AreEventsEnabled() const { return events_enabled_; }

    /**
     * @brief Sets the flag indicating whether a visual transition is in progress.
     * When true, event processing might be skipped or handled differently.
     * @param inProgress True if a transition is starting, false if it's ending.
     */
    void SetTransitionInProgress(const bool inProgress) { transition_in_progress_ = inProgress; }

    /**
     * @brief Checks if a visual transition is currently marked as in progress.
     * @return True if a transition is in progress, false otherwise.
     */
    bool IsInTransition() const { return transition_in_progress_; }

signals:
    /**
     * @brief Emitted when a significant window move event is detected after throttling.
     * @param new_position The new position of the window's top-left corner.
     * @param timestamp The timestamp (ms since epoch) when the event was processed.
     */
    void windowMoved(const QPointF &new_position, qint64 timestamp);

    /**
     * @brief Emitted periodically during window movement to provide samples for velocity calculation.
     * This signal is emitted less frequently than raw move events but potentially more often than windowMoved.
     * @param position The window position at the time of sampling.
     * @param timestamp The timestamp (ms since epoch) of the sample.
     */
    void movementSampleAdded(const QPointF &position, qint64 timestamp);

    /**
     * @brief Emitted when a significant resize event is detected after throttling.
     * @param old_size The size of the widget before the resize.
     * @param new_size The size of the widget after the resize.
     */
    void significantResizeDetected(const QSize &old_size, const QSize &new_size);

    /**
     * @brief Emitted during a resize event to request a specific "resizing" state in the Blob animation.
     */
    void resizeStateRequested();

    /**
     * @brief Emitted after a resize event to request resetting any temporary state timers.
     */
    void stateResetTimerRequested();

    /**
     * @brief Emitted when event processing is re-enabled after being disabled.
     */
    void eventsReEnabled();

private slots:
    /**
     * @brief Slot connected to the event_re_enable_timer_'s timeout signal.
     * Calls EnableEvents() to re-enable event processing.
     */
    void onEventReEnableTimeout();

private:
    /**
     * @brief Handles the logic for processing QMoveEvent received by the event filter.
     * Applies throttling based on distance and time delta before emitting signals.
     * @param move_event The QMoveEvent to handle.
     */
    void HandleMoveEvent(const QMoveEvent *move_event);

    // --- Member Variables ---

    /** @brief Pointer to the parent widget whose window events are monitored. */
    QWidget *parent_widget_;
    /** @brief Flag indicating whether event processing is currently enabled. */
    bool events_enabled_;
    /** @brief Flag indicating if a visual transition (e.g., animation) is in progress, potentially suppressing event handling. */
    bool transition_in_progress_;

    /** @brief Stores the last window position that was processed after throttling. */
    QPointF last_processed_position_;
    /** @brief Timestamp (ms since epoch) of the last processed move event. */
    qint64 last_processed_move_time_;
    /** @brief Timestamp (ms since epoch) of the last drag-related event (currently seems unused). */
    qint64 last_drag_event_time_;

    /** @brief Timer used for delayed re-enabling of events (currently seems unused, timeout connected, but timer never started). */
    QTimer event_re_enable_timer_;
    /** @brief Flag indicating if the window is currently being resized (used to potentially ignore move events during resize). */
    bool is_resizing_ = false;
};

#endif // BLOBEVENTHANDLER_H
