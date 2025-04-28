
#include "attachment_queue_manager.h"

AttachmentTask::AttachmentTask(const std::function<void()> &taskFunc, QObject *parent): QObject(parent), m_taskFunc(taskFunc) {
    setAutoDelete(false);
}

void AttachmentTask::run() {
    m_taskFunc();
    emit finished();
}

void AttachmentQueueManager::addTask(const std::function<void()> &taskFunc) {
    QMutexLocker locker(&m_mutex);

    // Tworzymy zadanie
    auto task = new AttachmentTask(taskFunc);
    connect(task, &AttachmentTask::finished, this, [this, task]() {
        QMutexLocker taskLocker(&m_mutex);
        m_activeTasks.removeOne(task);
        task->deleteLater();
        processQueue();
    });

    m_taskQueue.enqueue(task);
    processQueue();
}

AttachmentQueueManager::AttachmentQueueManager(QObject *parent): QObject(parent) {
    // Ograniczamy liczbę jednoczesnych zadań do połowy dostępnych wątków
    m_maxActiveTasks = qMax(1, QThreadPool::globalInstance()->maxThreadCount() / 2);
    qDebug() << "AttachmentQueueManager: Max active tasks:" << m_maxActiveTasks;
}

void AttachmentQueueManager::processQueue() {
    // Sprawdzamy czy możemy uruchomić nowe zadanie
    while (!m_taskQueue.isEmpty() && m_activeTasks.size() < m_maxActiveTasks) {
        AttachmentTask* task = m_taskQueue.dequeue();
        m_activeTasks.append(task);
        QThreadPool::globalInstance()->start(task);
    }
}
