#pragma once


#include <cstddef>
#include <memory>

#include "thread_pool.h"


namespace siren {

class Event;
class Loop;


namespace detail {

struct AsyncTask
  : ThreadPoolTask
{
    Event *event;
};

}


class Async final
{
public:
    inline explicit Async(Loop *, std::size_t = 0);
    inline Async(Async &&) noexcept;
    inline ~Async();
    inline Async &operator=(Async &&) noexcept;

    inline bool isValid() const noexcept;
    inline void executeTask(const std::function<void ()> &);
    inline void executeTask(std::function<void ()> &&);

private:
    typedef detail::AsyncTask Task;

    inline void initialize();
    inline void finalize();
    inline void move(Async *) noexcept;

    std::shared_ptr<ThreadPool> threadPool_;
    Loop *loop_;
    void *fiberHandle_;
    std::size_t taskCount_;
};

}


/*
 * #include "async-inl.h"
 */


#include <cassert>
#include <cerrno>
#include <cstdint>
#include <system_error>
#include <utility>

#include "event.h"
#include "loop.h"
#include "scope_guard.h"


namespace siren {

Async::Async(Loop *loop, std::size_t numberOfThreads)
  : threadPool_(std::make_shared<ThreadPool>(numberOfThreads)),
    loop_(loop),
    taskCount_(0)
{
    assert(loop != nullptr);
    initialize();
}


Async::Async(Async &&other) noexcept
  : threadPool_(std::move(other.threadPool_)),
    loop_(other.loop_),
    taskCount_(0)
{
    assert(other.taskCount_ == 0);
    other.move(this);
}


Async::~Async()
{
    assert(taskCount_ == 0);
    finalize();
}


Async &
Async::operator=(Async &&other) noexcept
{
    if (&other != this) {
        assert(taskCount_ == 0);
        assert(other.taskCount_ == 0);
        finalize();
        threadPool_ = std::move(other.threadPool_);
        loop_ = other.loop_;
        other.move(this);
    }

    return *this;
}


void
Async::initialize()
{
    loop_->registerFD(threadPool_->getEventFD());

    auto scopeGuard = MakeScopeGuard([this] () -> void {
        loop_->unregisterFD(threadPool_->getEventFD());
    });

    fiberHandle_ = loop_->createFiber([threadPool = threadPool_, loop = loop_] () -> void {
        for (;;) {
            std::uint64_t dummy;

            if (loop->read(threadPool->getEventFD(), &dummy, sizeof(dummy)) < 0) {
                throw std::system_error(errno, std::system_category(), "read() failed");
            }

            std::vector<ThreadPoolTask *> threadPoolTasks = threadPool->getCompletedTasks();

            for (ThreadPoolTask *threadPoolTask : threadPoolTasks) {
                auto task = static_cast<Task *>(threadPoolTask);
                task->event->trigger();
            }
        }
    }, 0, true);

    scopeGuard.dismiss();
}


void
Async::finalize()
{
    if (isValid()) {
        loop_->interruptFiber(fiberHandle_);
        loop_->unregisterFD(threadPool_->getEventFD());
    }
}


void
Async::move(Async *other) noexcept
{
    other->fiberHandle_ = fiberHandle_;
    fiberHandle_ = nullptr;
}


bool
Async::isValid() const noexcept
{
    return threadPool_ != nullptr && fiberHandle_ != nullptr;
}


void
Async::executeTask(const std::function<void ()> &procedure)
{
    assert(isValid());
    assert(procedure != nullptr);
    Task task;
    threadPool_->addTask(&task, procedure);
    Event event = loop_->makeEvent();
    task.event = &event;
    ++taskCount_;
    SIREN_DEFER_FIBER_INTERRUPTION(*loop_, event.waitFor());
    --taskCount_;
    task.check();

}


void
Async::executeTask(std::function<void ()> &&procedure)
{
    assert(isValid());
    assert(procedure != nullptr);
    Task task;
    threadPool_->addTask(&task, std::move(procedure));
    Event event = loop_->makeEvent();
    task.event = &event;
    ++taskCount_;
    SIREN_DEFER_FIBER_INTERRUPTION(*loop_, event.waitFor());
    --taskCount_;
    task.check();
}

}
