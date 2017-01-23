#pragma once


#include <cstddef>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "list.h"


namespace siren {

class ThreadPool;


namespace detail {

enum class ThreadPoolTaskState
{
    Initial,
    Uncompleted,
    Completed
};

}


class ThreadPoolTask
  : private ListNode
{
public:
    inline void check();

protected:
    inline explicit ThreadPoolTask() noexcept;
    inline ~ThreadPoolTask();

private:
    typedef detail::ThreadPoolTaskState State;

    std::atomic<State> state_;
    std::function<void ()> procedure_;
    std::exception_ptr exception_;

    ThreadPoolTask(const ThreadPoolTask &) = delete;
    ThreadPoolTask &operator=(const ThreadPoolTask &) = delete;

    friend ThreadPool;
};


class ThreadPool final
{
public:
    typedef ThreadPoolTask Task;

    inline explicit ThreadPool(std::size_t = 0);
    inline ~ThreadPool();

    inline int getEventFD() const noexcept;
    inline void removeTask(Task *);
    inline void getCompletedTasks(std::vector<Task *> *);

    template <class T>
    inline void addTask(Task *, T &&);

private:
    typedef detail::ThreadPoolTaskState TaskState;

    Task noTask_;
    List pendingTaskList_;
    List completedTaskList_;
    int eventFD_;
    std::mutex mutexes_[2];
    std::condition_variable conditionVariable_;
    std::vector<std::thread> threads_;

    inline void start(std::size_t);
    inline void stop();

    void initialize();
    void finalize();
    void worker();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
};

}


/*
 * #include "thread_pool-inl.h"
 */


#include <cassert>
#include <utility>

#include "scope_guard.h"


namespace siren {

ThreadPoolTask::ThreadPoolTask() noexcept
  : state_(State::Initial)
{
}


ThreadPoolTask::~ThreadPoolTask()
{
    assert(state_.load(std::memory_order_relaxed) == State::Initial);
}


void
ThreadPoolTask::check()
{
    assert(state_.load(std::memory_order_relaxed) == State::Completed);
    state_.store(State::Initial, std::memory_order_relaxed);
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


int
ThreadPool::getEventFD() const noexcept
{
    return eventFD_;
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
    {
        std::lock_guard<std::mutex> lockGuard(mutexes_[0]);
        pendingTaskList_.appendNode(&noTask_);
        conditionVariable_.notify_all();
    }

    for (std::thread &thread : threads_) {
        thread.join();
    }
}


template <class T>
void
ThreadPool::addTask(Task *task, T &&procedure)
{
    assert(task != nullptr);
    assert(task->state_.load(std::memory_order_relaxed) == TaskState::Initial);
    task->state_.store(TaskState::Uncompleted, std::memory_order_relaxed);
    task->procedure_ = std::forward<T>(procedure);

    {
        std::lock_guard<std::mutex> lockGuard(mutexes_[0]);
        pendingTaskList_.appendNode(task);
        conditionVariable_.notify_one();
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

    {
        std::unique_lock<std::mutex> uniqueLock(mutexes_[1]);
        task->remove();
    }
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

}
