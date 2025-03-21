#ifndef BLOBEVENTHANDLER_H
#define BLOBEVENTHANDLER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QPointF>
#include <QSize>
#include <QEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QDateTime>
#include <QVector2D>
#include <QDebug>

class BlobEventHandler : public QObject {
    Q_OBJECT

public:
    explicit BlobEventHandler(QWidget* parentWidget);
    ~BlobEventHandler();

    bool processEvent(QEvent* event);
    bool processResizeEvent(QResizeEvent* event);
    bool eventFilter(QObject* watched, QEvent* event) override;

    void enableEvents();
    void disableEvents();
    bool areEventsEnabled() const { return m_eventsEnabled; }

    void setTransitionInProgress(bool inProgress) { m_transitionInProgress = inProgress; }
    bool isInTransition() const { return m_transitionInProgress; }

    signals:
        // Sygnały dotyczące ruchu okna
        void windowMoved(const QPointF& newPosition, qint64 timestamp);
    void movementSampleAdded(const QPointF& position, qint64 timestamp);

    // Sygnały dotyczące zmiany rozmiaru
    void significantResizeDetected(const QSize& oldSize, const QSize& newSize);
    void resizeStateRequested();
    void stateResetTimerRequested();

    // Sygnał informujący o włączeniu eventów po opóźnieniu
    void eventsReEnabled();
    void resizeInProgress(bool inProgress);

    private slots:
        void onEventReEnableTimeout();

private:
    // Metody pomocnicze
    void handleMoveEvent(QMoveEvent* moveEvent);
    void handleDragMoveEvent();

    // Pola przechowujące stan
    QWidget* m_parentWidget;
    bool m_eventsEnabled;
    bool m_transitionInProgress;

    // Buforowanie ostatnich pozycji/czasów dla throttlingu
    QPointF m_lastProcessedPosition;
    qint64 m_lastProcessedMoveTime;
    qint64 m_lastDragEventTime;

    // Timer do ponownego włączenia eventów
    QTimer m_eventReEnableTimer;
    bool m_isResizing = false;
};

#endif // BLOBEVENTHANDLER_H