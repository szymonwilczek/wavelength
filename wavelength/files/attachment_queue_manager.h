#ifndef ATTACHMENT_QUEUE_MANAGER_H
#define ATTACHMENT_QUEUE_MANAGER_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QThreadPool>
#include <QRunnable>
#include <functional>
#include <QDebug>

class AttachmentTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    AttachmentTask(const std::function<void()>& taskFunc, QObject* parent = nullptr)
        : QObject(parent), m_taskFunc(taskFunc) {
        setAutoDelete(false);
    }

    void run() override {
        m_taskFunc();
        emit finished();
    }

signals:
    void finished();

private:
    std::function<void()> m_taskFunc;
};

class AttachmentQueueManager : public QObject {
    Q_OBJECT
public:
    static AttachmentQueueManager* getInstance() {
        static AttachmentQueueManager instance;
        return &instance;
    }

    void addTask(const std::function<void()>& taskFunc) {
        QMutexLocker locker(&m_mutex);
        
        // Tworzymy zadanie
        auto task = new AttachmentTask(taskFunc);
        connect(task, &AttachmentTask::finished, this, [this, task]() {
            QMutexLocker locker(&m_mutex);
            m_activeTasks.removeOne(task);
            task->deleteLater();
            processQueue();
        });

        m_taskQueue.enqueue(task);
        processQueue();
    }

private:
    AttachmentQueueManager(QObject* parent = nullptr) : QObject(parent) {
        // Ograniczamy liczbę jednoczesnych zadań do połowy dostępnych wątków
        m_maxActiveTasks = qMax(1, QThreadPool::globalInstance()->maxThreadCount() / 2);
        qDebug() << "AttachmentQueueManager: Max active tasks:" << m_maxActiveTasks;
    }

    void processQueue() {
        // Sprawdzamy czy możemy uruchomić nowe zadanie
        while (!m_taskQueue.isEmpty() && m_activeTasks.size() < m_maxActiveTasks) {
            AttachmentTask* task = m_taskQueue.dequeue();
            m_activeTasks.append(task);
            QThreadPool::globalInstance()->start(task);
        }
    }

    QQueue<AttachmentTask*> m_taskQueue;
    QList<AttachmentTask*> m_activeTasks;
    QMutex m_mutex;
    int m_maxActiveTasks;
};

#endif // ATTACHMENT_QUEUE_MANAGER_H