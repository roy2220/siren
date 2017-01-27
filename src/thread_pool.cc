#include "thread_pool.h"

#include <cerrno>
#include <cstdint>
#include <system_error>

#include <sys/eventfd.h>
#include <unistd.h>

#include "scope_guard.h"


namespace siren {

ThreadPoolTask::~ThreadPoolTask()
{
    assert(state_.load(std::memory_order_relaxed) == State::Initial);
}


void
ThreadPoolTask::check()
{
    assert(state_.load(std::memory_order_relaxed) == State::Completed);
#ifndef NDEBUG
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

    auto scopeGuard = MakeScopeGuard([this] () -> void {
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
ThreadPool::finalize()
{
    if (close(eventFD_) < 0 && errno != EINTR) {
        throw std::system_error(errno, std::system_category(), "close() failed");
    }
}


void
ThreadPool::start(std::size_t numberOfThreads)
{
    threads_.reserve(numberOfThreads);
    threads_.emplace_back(&ThreadPool::worker, this);

    auto scopeGuard = MakeScopeGuard([this] () -> void {
        stop();
    });

    while (--numberOfThreads >= 1) {
        threads_.emplace_back(&ThreadPool::worker, this);
    }

    scopeGuard.dismiss();
}


void
ThreadPool::stop()
{
    closeWaitingTaskList();

    for (std::thread &thread : threads_) {
        thread.join();
    }
}


void
ThreadPool::worker()
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
                        throw std::system_error(errno, std::system_category(), "write() failed");
                    }
                } else {
                    break;
                }
            }
        }
    }
}


void
ThreadPool::removeTask(Task *task)
{
    assert(task != nullptr);
    assert(task->state_.load(std::memory_order_relaxed) != TaskState::Initial);

    while (task->state_.load(std::memory_order_acquire) != TaskState::Completed) {
        std::this_thread::yield();
    }

    removeCompletedTask(task);
}


void
ThreadPool::addWaitingTask(Task *task)
{
    std::lock_guard<std::mutex> lockGuard(mutexes_[0]);
    waitingTaskList_.appendNode(task);
    conditionVariable_.notify_one();
}


ThreadPoolTask *
ThreadPool::getWaitingTask()
{
    std::unique_lock<std::mutex> uniqueLock(mutexes_[0]);

    while (waitingTaskList_.isEmpty()) {
        conditionVariable_.wait(uniqueLock);
    }

    Task *task = static_cast<ThreadPoolTask *>(waitingTaskList_.getTail());

    if (task == &noTask_) {
        return nullptr;
    } else {
        task->remove();
        return task;
    }
}


void
ThreadPool::closeWaitingTaskList()
{
    std::lock_guard<std::mutex> lockGuard(mutexes_[0]);
    waitingTaskList_.prependNode(&noTask_);
    conditionVariable_.notify_all();
}


void
ThreadPool::addCompletedTask(Task *task)
{
    std::unique_lock<std::mutex> uniqueLock(mutexes_[1]);
    completedTaskList_.appendNode(task);
}


void
ThreadPool::removeCompletedTask(Task *task)
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

    SIREN_LIST_FOREACH_REVERSE(ListNode, list) {
        auto task = static_cast<Task *>(ListNode);
        tasks->push_back(task);
    }
}

} // namespace siren
