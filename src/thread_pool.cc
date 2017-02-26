#include "thread_pool.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <system_error>

#include <sys/eventfd.h>
#include <unistd.h>

#include "config.h"
#include "scope_guard.h"


namespace siren {

ThreadPoolTask::~ThreadPoolTask()
{
    SIREN_ASSERT(state_.load(std::memory_order_relaxed) == State::Initial);
}


void
ThreadPoolTask::check()
{
    SIREN_ASSERT(state_.load(std::memory_order_relaxed) == State::Completed);
#ifdef SIREN_WITH_DEBUG
    state_.store(State::Initial, std::memory_order_relaxed);
#endif
    procedure_ = nullptr;

    if (exception_ != nullptr) {
        std::rethrow_exception(std::move(exception_));
    }
}


ThreadPool::ThreadPool(std::size_t numberOfThreads)
{
    initialize();

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        finalize();
    });

    if (numberOfThreads == 0) {
        numberOfThreads = std::thread::hardware_concurrency();
    }

    start(numberOfThreads);
    scopeGuard.dismiss();
}


ThreadPool::~ThreadPool()
{
    stop();
    finalize();
}


void
ThreadPool::initialize()
{
    eventFD_ = eventfd(0, 0);

    if (eventFD_ < 0) {
        throw std::system_error(errno, std::system_category(), "eventfd() failed");
    }
}


void
ThreadPool::finalize() noexcept
{
    if (close(eventFD_) < 0 && errno != EINTR) {
        std::perror("close() failed");
        std::terminate();
    }
}


void
ThreadPool::start(std::size_t numberOfThreads)
{
    threads_.reserve(numberOfThreads);
    threads_.emplace_back(&ThreadPool::worker, this);

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        stop();
    });

    while (--numberOfThreads >= 1) {
        threads_.emplace_back(&ThreadPool::worker, this);
    }

    scopeGuard.dismiss();
}


void
ThreadPool::stop() noexcept
{
    noMoreWaitingTasks();

    for (std::thread &thread : threads_) {
        thread.join();
    }
}


void
ThreadPool::worker() noexcept
{
    for (;;) {
        ThreadPoolTask *task = getWaitingTask();

        if (task == nullptr) {
            return;
        } else {
            try {
                task->procedure_();
            } catch (...) {
                task->exception_ = std::current_exception();
            }

            addCompletedTask(task);
            task->state_.store(TaskState::Completed, std::memory_order_release);

            for (;;) {
                std::uint64_t dummy = 1;

                if (write(eventFD_, &dummy, sizeof(dummy)) < 0) {
                    if (errno != EINTR) {
                        std::perror("write() failed");
                        std::terminate();
                    }
                } else {
                    break;
                }
            }
        }
    }
}


void
ThreadPool::removeTask(Task *task, bool *taskIsCompleted) noexcept
{
    SIREN_ASSERT(task != nullptr);
    SIREN_ASSERT(taskIsCompleted != nullptr);
    SIREN_ASSERT(task->state_.load(std::memory_order_relaxed) != TaskState::Initial);

    if (task->state_.load(std::memory_order_acquire) == TaskState::Uncompleted) {
        *taskIsCompleted = false;

        if (removeWaitingTask(task)) {
#ifdef SIREN_WITH_DEBUG
            task->state_.store(TaskState::Initial, std::memory_order_release);
#endif
            return;
        } else {
            while (task->state_.load(std::memory_order_acquire) == TaskState::Uncompleted) {
                std::this_thread::yield();
            }
        }
    }

    *taskIsCompleted = true;
    removeCompletedTask(task);
}


void
ThreadPool::addWaitingTask(Task *task)
{
    std::lock_guard<std::mutex> lockGuard(mutexes_[0]);
    waitingTaskList_.appendNode((task->isWaiting_ = true, task));
    conditionVariable_.notify_one();
}


bool
ThreadPool::removeWaitingTask(Task *task) noexcept
{
    std::unique_lock<std::mutex> uniqueLock(mutexes_[0]);

    if (task->isWaiting_) {
        task->remove();
        return true;
    } else {
        return false;
    }
}


ThreadPoolTask *
ThreadPool::getWaitingTask()
{
    std::unique_lock<std::mutex> uniqueLock(mutexes_[0]);

    while (waitingTaskList_.isEmpty()) {
        conditionVariable_.wait(uniqueLock);
    }

    ListNode *listNode = waitingTaskList_.getTail();

    if (listNode == &noListNode_) {
        return nullptr;
    } else {
        Task *task = static_cast<ThreadPoolTask *>(listNode);
        (task->isWaiting_ = false, task)->remove();
        return task;
    }
}


void
ThreadPool::noMoreWaitingTasks()
{
    std::lock_guard<std::mutex> lockGuard(mutexes_[0]);
    waitingTaskList_.prependNode(&noListNode_);
    conditionVariable_.notify_all();
}


void
ThreadPool::addCompletedTask(Task *task)
{
    std::unique_lock<std::mutex> uniqueLock(mutexes_[1]);
    completedTaskList_.appendNode(task);
}


void
ThreadPool::removeCompletedTask(Task *task) noexcept
{
    std::unique_lock<std::mutex> uniqueLock(mutexes_[1]);
    task->remove();
}


void
ThreadPool::getCompletedTasks(std::vector<ThreadPool::Task *> *tasks)
{
    List list;

    {
        std::lock_guard<std::mutex> lockGuard(mutexes_[1]);
        list = std::move(completedTaskList_);
    }

    Task *task;

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        std::lock_guard<std::mutex> lockGuard(mutexes_[1]);
        completedTaskList_.prependNodes(list.getHead(), task);
    });

    SIREN_LIST_FOREACH_REVERSE(ListNode, list) {
        task = static_cast<Task *>(ListNode);
        tasks->push_back(task);
    }

    scopeGuard.dismiss();
}

} // namespace siren
