
#include "attachment_queue_manager.h"

AttachmentTask::AttachmentTask(const std::function<void()> &taskFunc, QObject *parent): QObject(parent), TaskFunc_(taskFunc) {
    setAutoDelete(false);
}

void AttachmentTask::run() {
    TaskFunc_();
    emit finished();
}

void AttachmentQueueManager::AddTask(const std::function<void()> &TaskFunc) {
    QMutexLocker locker(&mutex_);

    // Tworzymy zadanie
    auto task = new AttachmentTask(TaskFunc);
    connect(task, &AttachmentTask::finished, this, [this, task]() {
        QMutexLocker taskLocker(&mutex_);
        active_tasks_.removeOne(task);
        task->deleteLater();
        ProcessQueue();
    });

    task_queue_.enqueue(task);
    ProcessQueue();
}

AttachmentQueueManager::AttachmentQueueManager(QObject *parent): QObject(parent) {
    // Ograniczamy liczbę jednoczesnych zadań do połowy dostępnych wątków
    max_active_tasks_ = qMax(1, QThreadPool::globalInstance()->maxThreadCount() / 2);
}

void AttachmentQueueManager::ProcessQueue() {
    // Sprawdzamy czy możemy uruchomić nowe zadanie
    while (!task_queue_.isEmpty() && active_tasks_.size() < max_active_tasks_) {
        AttachmentTask* task = task_queue_.dequeue();
        active_tasks_.append(task);
        QThreadPool::globalInstance()->start(task);
    }
}
