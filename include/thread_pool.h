#pragma once


#include <cstddef>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "list.h"


namespace siren {

class ThreadPool;


class ThreadPoolTask
  : private ListNode
{
public:
    inline void check();

protected:
    inline explicit ThreadPoolTask() noexcept;

    ~ThreadPoolTask() = default;

private:
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
    inline std::vector<Task *> getCompletedTasks() noexcept;

    template <class T>
    inline void addTask(Task *, T &&);

private:
    List pendingTaskList_;
    Task noTask_;
    std::vector<Task *> completedTasks_;
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
{
}


void
ThreadPoolTask::check()
{
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

    auto scopeGuard = MakeScopeGuard([this] () -> void {
        stop();
    });

    while (numberOfThreads >= 1) {
        threads_.emplace_back(&ThreadPool::worker, this);
        --numberOfThreads;
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
    task->procedure_ = std::forward<T>(procedure);

    {
        std::lock_guard<std::mutex> lockGuard(mutexes_[0]);
        pendingTaskList_.appendNode(task);
        conditionVariable_.notify_one();
    }
}


std::vector<ThreadPool::Task *>
ThreadPool::getCompletedTasks() noexcept
{
    std::vector<ThreadPool::Task *> tasks;

    {
        std::lock_guard<std::mutex> lockGuard(mutexes_[1]);
        tasks = std::move(completedTasks_);
    }

    return tasks;
}

}
