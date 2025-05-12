#include "blob_event_handler.h"

#include <QDateTime>
#include <QResizeEvent>
#include <QWidget>

BlobEventHandler::BlobEventHandler(QWidget *parent_widget)
    : QObject(parent_widget),
      parent_widget_(parent_widget),
      events_enabled_(true),
      transition_in_progress_(false),
      last_processed_position_(0, 0),
      last_processed_move_time_(0) {
    event_re_enable_timer_.setSingleShot(true);
    connect(&event_re_enable_timer_, &QTimer::timeout,
            this, &BlobEventHandler::onEventReEnableTimeout);

    if (parent_widget_ && parent_widget_->window()) {
        parent_widget_->window()->installEventFilter(this);
    }
}

BlobEventHandler::~BlobEventHandler() {
    if (parent_widget_ && parent_widget_->window()) {
        parent_widget_->window()->removeEventFilter(this);
    }
}

bool BlobEventHandler::ProcessResizeEvent(const QResizeEvent *event) {
    if (!events_enabled_) return false;
    if (!events_enabled_ || transition_in_progress_) {
        return false;
    }

    const QSize current_size = parent_widget_->size();
    const QSize old_size = event->oldSize();

    constexpr int min_dimension_change = 3; // min. change in px to react
    const bool significant_change =
            abs(current_size.width() - old_size.width()) > min_dimension_change ||
            abs(current_size.height() - old_size.height()) > min_dimension_change;

    static qint64 last_resize_time = 0;

    // throttling
    if (const qint64 current_time = QDateTime::currentMSecsSinceEpoch();
        significant_change && current_time - last_resize_time >= 16) {
        last_resize_time = current_time;

        emit resizeStateRequested();
        emit significantResizeDetected(old_size, current_size);
        emit stateResetTimerRequested();

        return true;
    }
    return false;
}

void BlobEventHandler::HandleMoveEvent(const QMoveEvent *move_event) {
    if (is_resizing_) {
        return;
    }

    const QPointF new_position = move_event->pos();
    const qint64 current_time = QDateTime::currentMSecsSinceEpoch();
    const double move_distance = QVector2D(new_position - last_processed_position_).length();
    const qint64 delta_time = current_time - last_processed_move_time_;
    double distance_threshold = 5.0;

    if (delta_time < 8) {
        return; // omit small movements
    }
    if (delta_time < 16) {
        distance_threshold = 7.0;
    }

    if (move_distance > distance_threshold || delta_time > 16) {
        last_processed_position_ = new_position;
        last_processed_move_time_ = current_time;

        if (const QWidget *current_window = parent_widget_->window()) {
            const QPointF window_position = current_window->pos();
            emit windowMoved(window_position, current_time);

            if (constexpr int timeThreshold = 16; delta_time > timeThreshold) {
                emit movementSampleAdded(window_position, current_time);
            }
        }
    }
}

bool BlobEventHandler::eventFilter(QObject *watched, QEvent *event) {
    if (!events_enabled_) return false;
    if (!events_enabled_ || transition_in_progress_) {
        return QObject::eventFilter(watched, event);
    }

    if (watched == parent_widget_->window()) {
        if (event->type() == QEvent::Move) {
            HandleMoveEvent(dynamic_cast<QMoveEvent *>(event));
        }
    }

    return QObject::eventFilter(watched, event);
}

void BlobEventHandler::EnableEvents() {
    events_enabled_ = true;
    emit eventsReEnabled();
}

void BlobEventHandler::DisableEvents() {
    events_enabled_ = false;
}

void BlobEventHandler::onEventReEnableTimeout() {
    EnableEvents();
    emit eventsReEnabled();
}
