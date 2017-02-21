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
namespace detail { enum class ThreadPoolTaskState; }


class ThreadPoolTask
  : private ListNode
{
public:
    void check();

protected:
    inline explicit ThreadPoolTask() noexcept;

    ~ThreadPoolTask();

private:
    typedef detail::ThreadPoolTaskState State;

    std::atomic<State> state_;
    bool isWaiting_;
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

    inline int getEventFD() const noexcept;

    template <class T>
    inline void addTask(Task *, T &&);

    explicit ThreadPool(std::size_t = 0);
    ~ThreadPool();

    void removeTask(Task *, bool *) noexcept;
    void getCompletedTasks(std::vector<Task *> *);

private:
    typedef detail::ThreadPoolTaskState TaskState;

    Task noListNode_;
    List waitingTaskList_;
    List completedTaskList_;
    int eventFD_;
    std::mutex mutexes_[2];
    std::condition_variable conditionVariable_;
    std::vector<std::thread> threads_;

    void initialize();
    void finalize() noexcept;
    void start(std::size_t);
    void stop() noexcept;
    void worker() noexcept;
    void addWaitingTask(Task *);
    bool removeWaitingTask(Task *) noexcept;
    Task *getWaitingTask();
    void noMoreWaitingTasks();
    void addCompletedTask(Task *);
    void removeCompletedTask(Task *) noexcept;

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
};

} // namespace siren


/*
 * #include "thread_pool-inl.h"
 */


#include <cassert>
#include <utility>


namespace siren {

namespace detail {

enum class ThreadPoolTaskState
{
#ifndef NDEBUG
    Initial,
#endif
    Uncompleted,
    Completed,
};

} // namespace detail



ThreadPoolTask::ThreadPoolTask() noexcept
#ifndef NDEBUG
  : state_(State::Initial)
#endif
{
}


int
ThreadPool::getEventFD() const noexcept
{
    return eventFD_;
}


template <class T>
void
ThreadPool::addTask(Task *task, T &&procedure)
{
    assert(task != nullptr);
    assert(task->state_.load(std::memory_order_relaxed) == TaskState::Initial);
    task->state_.store(TaskState::Uncompleted, std::memory_order_relaxed);
    task->procedure_ = std::forward<T>(procedure);
    addWaitingTask(task);
}

} // namespace siren
