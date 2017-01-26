#include "async.h"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <system_error>
#include <utility>

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
Async::EventTrigger(ThreadPool *threadPool, Loop *loop)
{
    std::vector<ThreadPoolTask *> threadPoolTasks;

    for (;;) {
        {
            std::uint64_t dummy;

            if (loop->read(threadPool->getEventFD(), &dummy, sizeof(dummy)) < 0) {
                throw std::system_error(errno, std::system_category(), "read() failed");
            }
        }

        threadPool->getCompletedTasks(&threadPoolTasks);

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

    fiberHandle_ = loop_->createFiber(std::bind(EventTrigger, threadPool_.get(), loop_), 0, true);
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


void
Async::executeTask(const std::function<void ()> &procedure)
{
    assert(isValid());
    assert(procedure != nullptr);
    Task task;
    threadPool_->addTask(&task, procedure);
    ++taskCount_;
    waitForTask(&task);
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
    ++taskCount_;
    waitForTask(&task);
    --taskCount_;
    task.check();
}


void
Async::waitForTask(Task *task) noexcept
{
    Event event = loop_->makeEvent();

    try {
        (task->event = &event)->waitFor();
    } catch (FiberInterruption) {
        threadPool_->removeTask(task);
        loop_->interruptFiber(loop_->getCurrentFiber());
    }
}

} // namespace siren
