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
    explicit BlobEventHandler(QWidget* parentWidget);
    ~BlobEventHandler() override;

    static bool processEvent();
    bool processResizeEvent(const QResizeEvent* event);
    bool eventFilter(QObject* watched, QEvent* event) override;

    void enableEvents();
    void disableEvents();
    bool areEventsEnabled() const { return m_eventsEnabled; }

    void setTransitionInProgress(const bool inProgress) { m_transitionInProgress = inProgress; }
    bool isInTransition() const { return m_transitionInProgress; }

    signals:
        void windowMoved(const QPointF& newPosition, qint64 timestamp);
        void movementSampleAdded(const QPointF& position, qint64 timestamp);

    void significantResizeDetected(const QSize& oldSize, const QSize& newSize);
    void resizeStateRequested();
    void stateResetTimerRequested();

    void eventsReEnabled();

    private slots:
        void onEventReEnableTimeout();

private:
    void handleMoveEvent(const QMoveEvent* moveEvent);

    QWidget* m_parentWidget;
    bool m_eventsEnabled;
    bool m_transitionInProgress;

    QPointF m_lastProcessedPosition;
    qint64 m_lastProcessedMoveTime;
    qint64 m_lastDragEventTime;

    QTimer m_eventReEnableTimer;
    bool m_isResizing = false;
};

#endif // BLOBEVENTHANDLER_H