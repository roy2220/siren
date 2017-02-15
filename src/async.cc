#include "async.h"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <system_error>
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


int
Async::getaddrinfo(const char *hostName, const char *serviceName, const addrinfo *hints
                   , addrinfo **result)
{
    struct {
        const char *hostName;
        const char *serviceName;
        const addrinfo *hints;
        addrinfo **result;
        int errorCode;
    } context;

    context.hostName = hostName;
    context.serviceName = serviceName;
    context.hints = hints;
    context.result = result;

    executeTask([&context] () -> void {
        context.errorCode = ::getaddrinfo(context.hostName, context.serviceName, context.hints
                                          , context.result);
    });

    return context.errorCode;
}


int
Async::getnameinfo(const sockaddr *name, socklen_t nameSize, char *hostName, socklen_t hostNameSize
                   , char *serviceName, socklen_t serviceNameSize, int flags)
{
    struct {
        const struct sockaddr *name;
        socklen_t nameSize;
        char *hostName;
        socklen_t hostNameSize;
        char *serviceName;
        socklen_t serviceNameSize;
        int flags;
        int errorCode;
    } context;

    context.name = name;
    context.nameSize = nameSize;
    context.hostName = hostName;
    context.hostNameSize = hostNameSize;
    context.serviceName = serviceName;
    context.serviceNameSize = serviceNameSize;
    context.flags = flags;

    executeTask([&context] () -> void {
        context.errorCode = ::getnameinfo(context.name, context.nameSize, context.hostName
                                          , context.hostNameSize, context.serviceName
                                          , context.serviceNameSize, context.flags);
    });

    return context.errorCode;
}


int
Async::open(const char *path, int flags, mode_t mode)
{
    struct {
        const char *path;
        int flags;
        mode_t mode;
        int fd;
    } context;

    context.path = path;
    context.flags = flags;
    context.mode = mode;

    executeTask([&context] () -> void {
        context.fd = ::open(context.path, context.flags, context.mode);
    });

    return context.fd;
}


ssize_t
Async::read(int fd, void *buffer, size_t bufferSize)
{
    struct {
        int fd;
        void *buffer;
        size_t bufferSize;
        ssize_t numberOfBytes;
    } context;

    context.fd = fd;
    context.buffer = buffer;
    context.bufferSize = bufferSize;

    executeTask([&context] () -> void {
        context.numberOfBytes = ::read(context.fd, context.buffer, context.bufferSize);
    });

    return context.numberOfBytes;
}


ssize_t
Async::write(int fd, const void *data, size_t dataSize)
{
    struct {
        int fd;
        const void *data;
        size_t dataSize;
        ssize_t numberOfBytes;
    } context;

    context.fd = fd;
    context.data = data;
    context.dataSize= dataSize;

    executeTask([&context] () -> void {
        context.numberOfBytes = ::write(context.fd, context.data, context.dataSize);
    });

    return context.numberOfBytes;
}


ssize_t
Async::readv(int fd, const iovec *vector, int vectorLength)
{
    struct {
        int fd;
        const iovec *vector;
        int vectorLength;
        ssize_t numberOfBytes;
    } context;

    context.fd = fd;
    context.vector = vector;
    context.vectorLength = vectorLength;

    executeTask([&context] () -> void {
        context.numberOfBytes = ::readv(context.fd, context.vector, context.vectorLength);
    });

    return context.numberOfBytes;
}


ssize_t
Async::writev(int fd, const iovec *vector, int vectorLength)
{
    struct {
        int fd;
        const iovec *vector;
        int vectorLength;
        ssize_t numberOfBytes;
    } context;

    context.fd = fd;
    context.vector = vector;
    context.vectorLength = vectorLength;

    executeTask([&context] () -> void {
        context.numberOfBytes = ::writev(context.fd, context.vector, context.vectorLength);
    });

    return context.numberOfBytes;
}


int
Async::close(int fd)
{
    struct {
        int fd;
        int result;
    } context;

    context.fd = fd;

    executeTask([&context] () -> void {
        context.result = ::close(context.fd);
    });

    return context.result;
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
