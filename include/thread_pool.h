#pragma once


#include <cstddef>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "config.h"
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
    inline std::enable_if_t<!std::is_same<T, nullptr_t>::value, void> addTask(Task *, T &&);

    template <class T>
    void removeCompletedTasks(T &&);

    explicit ThreadPool(std::size_t = 0);
    ~ThreadPool();

    void removeTask(Task *, bool *) noexcept;

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
    Task *removeWaitingTask() noexcept;
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


#include <utility>

#include "assert.h"
#include "scope_guard.h"


namespace siren {

namespace detail {

enum class ThreadPoolTaskState
{
#ifdef SIREN_WITH_DEBUG
    Initial,
#endif
    Uncompleted,
    Completed,
};

} // namespace detail



ThreadPoolTask::ThreadPoolTask() noexcept
#ifdef SIREN_WITH_DEBUG
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
std::enable_if_t<!std::is_same<T, nullptr_t>::value, void>
ThreadPool::addTask(Task *task, T &&procedure)
{
    SIREN_ASSERT(task != nullptr);
    SIREN_ASSERT(task->state_.load(std::memory_order_relaxed) == TaskState::Initial);
    task->state_.store(TaskState::Uncompleted, std::memory_order_relaxed);
    task->procedure_ = std::forward<T>(procedure);
    addWaitingTask(task);
}


template <class T>
void
ThreadPool::removeCompletedTasks(T &&callback)
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

    SIREN_LIST_FOREACH_REVERSE(listNode, list) {
        task = static_cast<Task *>(listNode);
        callback(task);
    }

    scopeGuard.dismiss();
}

} // namespace siren
