#include "blob_event_handler.h"

#include <QDateTime>

BlobEventHandler::BlobEventHandler(QWidget* parentWidget)
    : QObject(parentWidget),
      m_parentWidget(parentWidget),
      m_eventsEnabled(true),
      m_transitionInProgress(false),
      m_lastProcessedPosition(0, 0),
      m_lastProcessedMoveTime(0),
      m_lastDragEventTime(0)
{
    m_eventReEnableTimer.setSingleShot(true);
    connect(&m_eventReEnableTimer, &QTimer::timeout, 
            this, &BlobEventHandler::onEventReEnableTimeout);
    
    // Instalacja filtra eventów na oknie nadrzędnym
    if (m_parentWidget && m_parentWidget->window()) {
        m_parentWidget->window()->installEventFilter(this);
    }
}

BlobEventHandler::~BlobEventHandler() {
    // Usunięcie filtra eventów z okna nadrzędnego przy zniszczeniu
    if (m_parentWidget && m_parentWidget->window()) {
        m_parentWidget->window()->removeEventFilter(this);
    }
}

bool BlobEventHandler::processEvent() {
    // Ta metoda może być używana dla zdarzeń ogólnych
    // obecnie nie jest używana, ale może być przydatna w przyszłości
    return false;  // przekazanie zdarzenia dalej
}

bool BlobEventHandler::processResizeEvent(const QResizeEvent* event) {
    if (!m_eventsEnabled) return false;
    if (!m_eventsEnabled || m_transitionInProgress) {
        return false;
    }

    // Pobierz aktualny rozmiar
    const QSize currentSize = m_parentWidget->size();
    const QSize oldSize = event->oldSize();

    // Sprawdź, czy zmiana rozmiaru jest znacząca
    constexpr int minDimChange = 3; // minimalna zmiana w pikselach, żeby reagować
    const bool significantChange =
        abs(currentSize.width() - oldSize.width()) > minDimChange ||
        abs(currentSize.height() - oldSize.height()) > minDimChange;

    static qint64 lastResizeTime = 0;

    // Zastosuj throttling - przetwarzaj resize tylko co 16ms (~60 FPS)
    if (const qint64 currentTime = QDateTime::currentMSecsSinceEpoch(); significantChange && (currentTime - lastResizeTime >= 16)) {
        lastResizeTime = currentTime;


        // Emituj sygnał o wykryciu znaczącej zmiany rozmiaru
        emit resizeStateRequested();
        emit significantResizeDetected(oldSize, currentSize);

        // Resetuj timer stanu tylko po zakończeniu resize
        emit stateResetTimerRequested();

        return true;
    }

    return false;
}

void BlobEventHandler::handleMoveEvent(const QMoveEvent* moveEvent) {
    if (m_isResizing) {
        return;
    }

    const QPointF newPos = moveEvent->pos();
    const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // Zwiększone progi dla lepszej wydajności
    const double moveDist = QVector2D(newPos - m_lastProcessedPosition).length();
    const qint64 timeDelta = currentTime - m_lastProcessedMoveTime;

    // Adaptacyjny próg odległości
    double distThreshold = 5.0;

    // Bardziej agresywne filtrowanie przy szybkim ruchu
    if (timeDelta < 8) {
        return; // Pomijamy zbyt częste zdarzenia
    }
    if (timeDelta < 16) {
        distThreshold = 7.0;
    }

    // Przetwarzaj tylko jeśli ruch jest znaczący lub upłynęło wystarczająco dużo czasu
    if (moveDist > distThreshold || timeDelta > 16) { // 33ms ~ 30 FPS minimalnie
        m_lastProcessedPosition = newPos;
        m_lastProcessedMoveTime = currentTime;

        if (const QWidget* currentWindow = m_parentWidget->window()) {
            const QPointF windowPos = currentWindow->pos();
            emit windowMoved(windowPos, currentTime);

            if (constexpr int timeThreshold = 16; timeDelta > timeThreshold) {
                emit movementSampleAdded(windowPos, currentTime);
            }
        }
    }
}


bool BlobEventHandler::eventFilter(QObject* watched, QEvent* event) {
    if (!m_eventsEnabled) return false;
    if (!m_eventsEnabled || m_transitionInProgress) {
        return QObject::eventFilter(watched, event);
    }

    if (watched == m_parentWidget->window()) {
        if (event->type() == QEvent::Move) {
            handleMoveEvent(dynamic_cast<QMoveEvent*>(event));
        }
    }

    return QObject::eventFilter(watched, event);
}

void BlobEventHandler::enableEvents() {
    m_eventsEnabled = true;
    emit eventsReEnabled();
}

void BlobEventHandler::disableEvents() {
    m_eventsEnabled = false;
}

void BlobEventHandler::onEventReEnableTimeout() {
    enableEvents();
    emit eventsReEnabled();
}