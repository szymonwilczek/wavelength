#include "blob_event_handler.h"

#include <QDateTime>

BlobEventHandler::BlobEventHandler(QWidget* parent_widget)
    : QObject(parent_widget),
      parent_widget_(parent_widget),
      events_enabled_(true),
      transition_in_progress_(false),
      last_processed_position_(0, 0),
      last_processed_move_time_(0),
      last_drag_event_time_(0)
{
    event_re_enable_timer_.setSingleShot(true);
    connect(&event_re_enable_timer_, &QTimer::timeout,
            this, &BlobEventHandler::onEventReEnableTimeout);
    
    // Instalacja filtra eventów na oknie nadrzędnym
    if (parent_widget_ && parent_widget_->window()) {
        parent_widget_->window()->installEventFilter(this);
    }
}

BlobEventHandler::~BlobEventHandler() {
    // Usunięcie filtra eventów z okna nadrzędnego przy zniszczeniu
    if (parent_widget_ && parent_widget_->window()) {
        parent_widget_->window()->removeEventFilter(this);
    }
}

bool BlobEventHandler::ProcessEvent() {
    // Ta metoda może być używana dla zdarzeń ogólnych
    // obecnie nie jest używana, ale może być przydatna w przyszłości
    return false;  // przekazanie zdarzenia dalej
}

bool BlobEventHandler::ProcessResizeEvent(const QResizeEvent* event) {
    if (!events_enabled_) return false;
    if (!events_enabled_ || transition_in_progress_) {
        return false;
    }

    // Pobierz aktualny rozmiar
    const QSize current_size = parent_widget_->size();
    const QSize old_size = event->oldSize();

    // Sprawdź, czy zmiana rozmiaru jest znacząca
    constexpr int min_dimension_change = 3; // minimalna zmiana w pikselach, żeby reagować
    const bool significant_change =
        abs(current_size.width() - old_size.width()) > min_dimension_change ||
        abs(current_size.height() - old_size.height()) > min_dimension_change;

    static qint64 last_resize_time = 0;

    // Zastosuj throttling - przetwarzaj resize tylko co 16ms (~60 FPS)
    if (const qint64 current_time = QDateTime::currentMSecsSinceEpoch(); significant_change && (current_time - last_resize_time >= 16)) {
        last_resize_time = current_time;


        // Emituj sygnał o wykryciu znaczącej zmiany rozmiaru
        emit resizeStateRequested();
        emit significantResizeDetected(old_size, current_size);

        // Resetuj timer stanu tylko po zakończeniu resize
        emit stateResetTimerRequested();

        return true;
    }

    return false;
}

void BlobEventHandler::HandleMoveEvent(const QMoveEvent* move_event) {
    if (is_resizing_) {
        return;
    }

    const QPointF new_position = move_event->pos();
    const qint64 current_time = QDateTime::currentMSecsSinceEpoch();

    // Zwiększone progi dla lepszej wydajności
    const double move_distance = QVector2D(new_position - last_processed_position_).length();
    const qint64 delta_time = current_time - last_processed_move_time_;

    // Adaptacyjny próg odległości
    double distance_treshold = 5.0;

    // Bardziej agresywne filtrowanie przy szybkim ruchu
    if (delta_time < 8) {
        return; // Pomijamy zbyt częste zdarzenia
    }
    if (delta_time < 16) {
        distance_treshold = 7.0;
    }

    // Przetwarzaj tylko jeśli ruch jest znaczący lub upłynęło wystarczająco dużo czasu
    if (move_distance > distance_treshold || delta_time > 16) { // 33ms ~ 30 FPS minimalnie
        last_processed_position_ = new_position;
        last_processed_move_time_ = current_time;

        if (const QWidget* current_window = parent_widget_->window()) {
            const QPointF window_position = current_window->pos();
            emit windowMoved(window_position, current_time);

            if (constexpr int timeThreshold = 16; delta_time > timeThreshold) {
                emit movementSampleAdded(window_position, current_time);
            }
        }
    }
}


bool BlobEventHandler::eventFilter(QObject* watched, QEvent* event) {
    if (!events_enabled_) return false;
    if (!events_enabled_ || transition_in_progress_) {
        return QObject::eventFilter(watched, event);
    }

    if (watched == parent_widget_->window()) {
        if (event->type() == QEvent::Move) {
            HandleMoveEvent(dynamic_cast<QMoveEvent*>(event));
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