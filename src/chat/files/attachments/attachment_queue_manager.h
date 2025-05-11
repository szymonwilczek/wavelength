#ifndef ATTACHMENT_QUEUE_MANAGER_H
#define ATTACHMENT_QUEUE_MANAGER_H

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QRunnable>

/**
 * @brief Represents a single task (a function) to be executed in a thread pool.
 *
 * This class wraps a std::function and implements QRunnable, allowing it to be
 * managed by QThreadPool. It emits a signal when the task function completes.
 */
class AttachmentTask final : public QObject, public QRunnable {
    Q_OBJECT

public:
    /**
     * @brief Constructs an AttachmentTask.
     * @param taskFunc The std::function to be executed when run() is called.
     * @param parent Optional parent QObject.
     */
    explicit AttachmentTask(const std::function<void()> &taskFunc, QObject *parent = nullptr);

    /**
     * @brief Executes the stored task function (TaskFunc_).
     * Called by the QThreadPool when the task is started. Emits finished() upon completion.
     */
    void run() override;

signals:
    /**
     * @brief Emitted after the task function (TaskFunc_) has finished executing.
     */
    void finished();

private:
    /** @brief The function object that represents the task to be executed. */
    std::function<void()> TaskFunc_;
};

/**
 * @brief Manages a queue of AttachmentTasks for concurrent execution using a singleton pattern.
 *
 * This class limits the number of concurrently running tasks (typically related to
 * loading or processing attachments) to avoid overwhelming system resources.
 * It uses QThreadPool to manage the execution of tasks.
 */
class AttachmentQueueManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the AttachmentQueueManager.
     * @return Pointer to the singleton AttachmentQueueManager instance.
     */
    static AttachmentQueueManager *GetInstance() {
        static AttachmentQueueManager instance;
        return &instance;
    }

    /**
     * @brief Adds a new task (represented by a std::function) to the execution queue.
     * Creates an AttachmentTask wrapper for the function, adds it to the queue,
     * and attempts to process the queue immediately. This operation is thread-safe.
     * @param TaskFunc The function to be executed as a task.
     */
    void AddTask(const std::function<void()> &TaskFunc);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * Determines the maximum number of concurrent tasks based on available system threads.
     * @param parent Optional parent QObject.
     */
    explicit AttachmentQueueManager(QObject *parent = nullptr);

    /**
     * @brief Processes the task queue, starting new tasks if slots are available.
     * Checks if the number of active tasks is below the maximum limit and if the queue
     * is not empty. If conditions are met, dequeues a task and starts it using the global QThreadPool.
     * This operation is protected by the internal mutex.
     */
    void ProcessQueue();

    /** @brief Queue storing tasks waiting to be executed. Access protected by mutex_. */
    QQueue<AttachmentTask *> task_queue_;
    /** @brief List storing tasks currently being executed by the thread pool. Access protected by mutex_. */
    QList<AttachmentTask *> active_tasks_;
    /** @brief Mutex ensuring thread-safe access to task_queue_ and active_tasks_. */
    QMutex mutex_;
    /** @brief Maximum number of tasks allowed to run concurrently. */
    int max_active_tasks_;
};

#endif // ATTACHMENT_QUEUE_MANAGER_H
