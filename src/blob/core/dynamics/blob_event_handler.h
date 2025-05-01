#ifndef BLOBEVENTHANDLER_H
#define BLOBEVENTHANDLER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QPointF>
#include <QMoveEvent>
#include <QDebug>

class BlobEventHandler final : public QObject {
    Q_OBJECT

public:
    explicit BlobEventHandler(QWidget* parent_widget);
    ~BlobEventHandler() override;

    static bool ProcessEvent();
    bool ProcessResizeEvent(const QResizeEvent* event);
    bool eventFilter(QObject* watched, QEvent* event) override;

    void EnableEvents();
    void DisableEvents();
    bool AreEventsEnabled() const { return events_enabled_; }

    void SetTransitionInProgress(const bool inProgress) { transition_in_progress_ = inProgress; }
    bool IsInTransition() const { return transition_in_progress_; }

    signals:
        void windowMoved(const QPointF& new_position, qint64 timestamp);
        void movementSampleAdded(const QPointF& position, qint64 timestamp);
        void significantResizeDetected(const QSize& old_size, const QSize& new_size);
        void resizeStateRequested();
        void stateResetTimerRequested();
        void eventsReEnabled();

    private slots:
        void onEventReEnableTimeout();

private:
    void HandleMoveEvent(const QMoveEvent* move_event);

    QWidget* parent_widget_;
    bool events_enabled_;
    bool transition_in_progress_;

    QPointF last_processed_position_;
    qint64 last_processed_move_time_;
    qint64 last_drag_event_time_;

    QTimer event_re_enable_timer_;
    bool is_resizing_ = false;
};

#endif // BLOBEVENTHANDLER_H