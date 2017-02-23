#include "async.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <utility>

#include <fcntl.h>

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


int
Async::getaddrinfo(const char *arg1, const char *arg2, const addrinfo *arg3, addrinfo **arg4)
{
    struct {
        const char *arg1;
        const char *arg2;
        const addrinfo *arg3;
        addrinfo **arg4;
        int ret;
    } context;

    context.arg1 = arg1;
    context.arg2 = arg2;
    context.arg3 = arg3;
    context.arg4 = arg4;

    executeTask([&context] () -> void {
        context.ret = ::getaddrinfo(context.arg1, context.arg2, context.arg3, context.arg4);
    });

    return context.ret;
}


int
Async::getnameinfo(const sockaddr *arg1, socklen_t arg2, char *arg3, socklen_t arg4
                   , char *arg5, socklen_t arg6, int arg7)
{
    struct {
        const struct sockaddr *arg1;
        socklen_t arg2;
        char *arg3;
        socklen_t arg4;
        char *arg5;
        socklen_t arg6;
        int arg7;
        int ret;
    } context;

    context.arg1 = arg1;
    context.arg2 = arg2;
    context.arg3 = arg3;
    context.arg4 = arg4;
    context.arg5 = arg5;
    context.arg6 = arg6;
    context.arg7 = arg7;

    executeTask([&context] () -> void {
        context.ret = ::getnameinfo(context.arg1, context.arg2, context.arg3, context.arg4
                                    , context.arg5, context.arg6, context.arg7);
    });

    return context.ret;
}


int
Async::open(const char *arg1, int arg2, mode_t arg3)
{
    struct {
        const char *arg1;
        int arg2;
        mode_t arg3;
        int ret;
    } context;

    context.arg1 = arg1;
    context.arg2 = arg2;
    context.arg3 = arg3;

    executeTask([&context] () -> void {
        context.ret = ::open(context.arg1, context.arg2, context.arg3);
    });

    return context.ret;
}


ssize_t
Async::read(int arg1, void *arg2, size_t arg3)
{
    struct {
        int arg1;
        void *arg2;
        size_t arg3;
        ssize_t ret;
    } context;

    context.arg1 = arg1;
    context.arg2 = arg2;
    context.arg3 = arg3;

    executeTask([&context] () -> void {
        context.ret = ::read(context.arg1, context.arg2, context.arg3);
    });

    return context.ret;
}


ssize_t
Async::write(int arg1, const void *arg2, size_t arg3)
{
    struct {
        int arg1;
        const void *arg2;
        size_t arg3;
        ssize_t ret;
    } context;

    context.arg1 = arg1;
    context.arg2 = arg2;
    context.arg3 = arg3;

    executeTask([&context] () -> void {
        context.ret = ::write(context.arg1, context.arg2, context.arg3);
    });

    return context.ret;
}


ssize_t
Async::readv(int arg1, const iovec *arg2, int arg3)
{
    struct {
        int arg1;
        const iovec *arg2;
        int arg3;
        ssize_t ret;
    } context;

    context.arg1 = arg1;
    context.arg2 = arg2;
    context.arg3 = arg3;

    executeTask([&context] () -> void {
        context.ret = ::readv(context.arg1, context.arg2, context.arg3);
    });

    return context.ret;
}


ssize_t
Async::writev(int arg1, const iovec *arg2, int arg3)
{
    struct {
        int arg1;
        const iovec *arg2;
        int arg3;
        ssize_t ret;
    } context;

    context.arg1 = arg1;
    context.arg2 = arg2;
    context.arg3 = arg3;

    executeTask([&context] () -> void {
        context.ret = ::writev(context.arg1, context.arg2, context.arg3);
    });

    return context.ret;
}


off_t
Async::lseek(int arg1, off_t arg2, int arg3)
{
    struct {
	int arg1;
	off_t arg2;
	int arg3;
        off_t ret;
    } context;

    context.arg1 = arg1;
    context.arg2 = arg2;
    context.arg3 = arg3;

    executeTask([&context] () -> void {
        context.ret = ::lseek(context.arg1, context.arg2, context.arg3);
    });

    return context.ret;
}


int
Async::close(int arg1)
{
    struct {
        int arg1;
        int ret;
    } context;

    context.arg1 = arg1;

    executeTask([&context] () -> void {
        context.ret = ::close(context.arg1);
    });

    return context.ret;
}


void
Async::executeTask(const std::function<void ()> &procedure)
{
    assert(isValid());
    assert(procedure != nullptr);
    Task task;
    threadPool_->addTask(&task, procedure);
    waitForTask(&task);
    task.check();
}


void
Async::executeTask(std::function<void ()> &&procedure)
{
    assert(isValid());
    assert(procedure != nullptr);
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
