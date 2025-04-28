#ifndef ATTACHMENT_QUEUE_MANAGER_H
#define ATTACHMENT_QUEUE_MANAGER_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QThreadPool>
#include <QRunnable>
#include <functional>
#include <QDebug>

class AttachmentTask final : public QObject, public QRunnable {
    Q_OBJECT
public:
    explicit AttachmentTask(const std::function<void()>& taskFunc, QObject* parent = nullptr);

    void run() override;

signals:
    void finished();

private:
    std::function<void()> m_taskFunc;
};

class AttachmentQueueManager final : public QObject {
    Q_OBJECT
public:
    static AttachmentQueueManager* getInstance() {
        static AttachmentQueueManager instance;
        return &instance;
    }

    void addTask(const std::function<void()>& taskFunc);

private:
    explicit AttachmentQueueManager(QObject* parent = nullptr);

    void processQueue();

    QQueue<AttachmentTask*> m_taskQueue;
    QList<AttachmentTask*> m_activeTasks;
    QMutex m_mutex;
    int m_maxActiveTasks;
};

#endif // ATTACHMENT_QUEUE_MANAGER_H