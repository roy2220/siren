#include "async.h"

#include <cstdint>
#include <cstdio>
#include <exception>

#include "assert.h"
#include "event.h"
#include "loop.h"
#include "scope_guard.h"


namespace siren {

namespace detail {

struct AsyncTask
  : ThreadPoolTask
{
    Event *event;
};

} // namespace detail


void
Async::EventTrigger(ThreadPool *threadPool, Loop *loop) noexcept
{
    std::vector<ThreadPoolTask *> threadPoolTasks;

    for (;;) {
        try {
            std::uint64_t dummy;

            if (loop->read(threadPool->getEventFD(), &dummy, sizeof(dummy)) < 0) {
                std::perror("read() failed");
                std::terminate();
            }
        } catch (FiberInterruption) {
            return;
        }

        threadPool->removeCompletedTasks(&threadPoolTasks);

        for (ThreadPoolTask *threadPoolTask : threadPoolTasks) {
            auto task = static_cast<Task *>(threadPoolTask);
            task->event->trigger();
        }

        threadPoolTasks.clear();
    }
}


Async::Async(Loop *loop, std::size_t numberOfThreads)
  : threadPool_(std::make_unique<ThreadPool>(numberOfThreads)),
    loop_(loop),
    taskCount_(0)
{
    SIREN_ASSERT(loop != nullptr);
    initialize();
}


Async::Async(Async &&other) noexcept
  : threadPool_(std::move(other.threadPool_)),
    loop_(other.loop_),
    taskCount_(0)
{
    SIREN_ASSERT(other.taskCount_ == 0);
    other.move(this);
}


Async::~Async()
{
    SIREN_ASSERT(taskCount_ == 0);
    finalize();
}


Async &
Async::operator=(Async &&other) noexcept
{
    if (&other != this) {
        SIREN_ASSERT(taskCount_ == 0);
        SIREN_ASSERT(other.taskCount_ == 0);
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

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        loop_->unregisterFD(threadPool_->getEventFD());
    });

    fiberHandle_ = loop_->createFiber(std::bind(EventTrigger, threadPool_.get(), loop_), 0, true);
    scopeGuard.dismiss();
}


void
Async::finalize() noexcept
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


void
Async::executeTask(const std::function<void ()> &procedure)
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(procedure != nullptr);
    Task task;
    threadPool_->addTask(&task, procedure);
    waitForTask(&task);
    task.check();
}


void
Async::executeTask(std::function<void ()> &&procedure)
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(procedure != nullptr);
    Task task;
    threadPool_->addTask(&task, std::move(procedure));
    waitForTask(&task);
    task.check();
}


void
Async::waitForTask(Task *task)
{
    Event event = loop_->makeEvent();
    ++taskCount_;

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        --taskCount_;
    });

    try {
        (task->event = &event)->waitFor();
    } catch (FiberInterruption) {
        bool taskIsCompleted;
        threadPool_->removeTask(task, &taskIsCompleted);

        if (!taskIsCompleted) {
            throw;
        }

        loop_->interruptFiber(loop_->getCurrentFiber());
    }
}

} // namespace siren
