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
    std::function<void()> TaskFunc_;
};

class AttachmentQueueManager final : public QObject {
    Q_OBJECT
public:
    static AttachmentQueueManager* GetInstance() {
        static AttachmentQueueManager instance;
        return &instance;
    }

    void AddTask(const std::function<void()>& TaskFunc);

private:
    explicit AttachmentQueueManager(QObject* parent = nullptr);

    void ProcessQueue();

    QQueue<AttachmentTask*> task_queue_;
    QList<AttachmentTask*> active_tasks_;
    QMutex mutex_;
    int max_active_tasks_;
};

#endif // ATTACHMENT_QUEUE_MANAGER_H