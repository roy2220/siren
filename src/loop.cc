#include "loop.h"

#include <cerrno>
#include <functional>
#include <system_error>
#include <utility>

#include <fcntl.h>

#include "helper_macros.h"
#include "scope_guard.h"


namespace siren {

namespace {

struct MyIOWatcher
  : IOWatcher
{
    std::function<void ()> callback;
};


struct MyIOTimer
  : IOTimer
{
    std::function<void ()> callback;
};


bool SetBlocking(int, bool);

} // namespace


Loop::Loop(std::size_t defaultFiberSize)
  : scheduler_(defaultFiberSize)
{
}


void
Loop::run()
{
    std::vector<IOWatcher *> ioWatchers;
    std::vector<IOTimer *> ioTimers;

    for (;;) {
        scheduler_.run();

        if (scheduler_.getNumberOfForegroundFibers() == 0) {
            break;
        } else {
            ioPoller_.getReadyWatchers(&ioClock_, &ioWatchers);

            for (IOWatcher *ioWatcher : ioWatchers) {
                auto myIOWatcher = static_cast<MyIOWatcher *>(ioWatcher);
                myIOWatcher->callback();
            }

            ioWatchers.clear();
            ioClock_.getExpiredTimers(&ioTimers);

            for (IOTimer *ioTimer : ioTimers) {
                auto myIOTimer = static_cast<MyIOTimer *>(ioTimer);
                myIOTimer->callback();
            }

            ioTimers.clear();
        }
    }
}


void
Loop::registerFD(int fd)
{
    {
        bool ok = SetBlocking(fd, false);
        SIREN_UNUSED(ok);
        assert(ok);
    }

    auto scopeGuard = MakeScopeGuard([fd] () -> void {
        SetBlocking(fd, true);
    });

    ioPoller_.createObject(fd);
    scopeGuard.dismiss();
}


void
Loop::unregisterFD(int fd)
{
    {
        bool ok = SetBlocking(fd, true);
        SIREN_UNUSED(ok);
        assert(ok);
    }

    auto scopeGuard = MakeScopeGuard([fd] () -> void {
        SetBlocking(fd, false);
    });

    ioPoller_.destroyObject(fd);
    scopeGuard.dismiss();
}


int
Loop::open(const char *path, int flags, mode_t mode)
{
    for (;;) {
        int fd = ::open(path, flags, mode | O_NONBLOCK);

        if (fd < 0) {
            if (errno != EINTR) {
                return -1;
            }
        } else {
            auto scopeGuard = MakeScopeGuard([fd] () -> void {
                if (::close(fd) < 0 && errno != EINTR) {
                    throw std::system_error(errno, std::system_category(), "close() failed");
                }
            });

            ioPoller_.createObject(fd);
            scopeGuard.dismiss();
            return fd;
        }
    }
}


int
Loop::pipe2(int fds[2], int flags)
{
    if (::pipe2(fds, flags | O_NONBLOCK) < 0) {
        return -1;
    } else {
        auto scopeGuard1 = MakeScopeGuard([fds] () -> void {
            if (::close(fds[0]) < 0 && errno != EINTR) {
                throw std::system_error(errno, std::system_category(), "close() failed");
            }

            if (::close(fds[1]) < 0 && errno != EINTR) {
                throw std::system_error(errno, std::system_category(), "close() failed");
            }
        });

        ioPoller_.createObject(fds[0]);

        auto scopeGuard2 = MakeScopeGuard([this, fds] () -> void {
            ioPoller_.destroyObject(fds[0]);
        });

        ioPoller_.createObject(fds[1]);
        scopeGuard1.dismiss();
        scopeGuard2.dismiss();
        return 0;
    }
}


ssize_t
Loop::read(int fd, void *buffer, size_t bufferSize, int timeout)
{
    return readFile(fd, timeout, ::read, buffer, bufferSize);
}


ssize_t
Loop::write(int fd, const void *data, size_t dataSize, int timeout)
{
    return writeFile(fd, timeout, ::write, data, dataSize);
}


ssize_t
Loop::readv(int fd, const iovec *vector, int vectorLength, int timeout)
{
    return readFile(fd, timeout, ::readv, vector, vectorLength);
}


ssize_t
Loop::writev(int fd, const iovec *vector, int vectorLength, int timeout)
{
    return writeFile(fd, timeout, ::writev, vector, vectorLength);
}


int
Loop::socket(int domain, int type, int protocol)
{
    int fd = ::socket(domain, type | SOCK_NONBLOCK, protocol);

    if (fd < 0) {
        return -1;
    } else {
        auto scopeGuard = MakeScopeGuard([fd] () -> void {
            if (::close(fd) < 0 && errno != EINTR) {
                throw std::system_error(errno, std::system_category(), "close() failed");
            }
        });

        ioPoller_.createObject(fd);
        scopeGuard.dismiss();
        return fd;
    }
}


int
Loop::accept4(int fd, sockaddr *name, socklen_t *nameSize, int flags, int timeout)
{
    for (;;) {
        int subFD = ::accept4(fd, name, nameSize, flags | SOCK_NONBLOCK);

        if (subFD < 0) {
            if (errno == EAGAIN) {
                if (!waitForFile(fd, IOCondition::Readable, std::chrono::milliseconds(timeout))) {
                    errno = EAGAIN;
                    return -1;
                }
            } else {
                if (errno != EINTR) {
                    return -1;
                }
            }
        } else {
            auto scopeGuard = MakeScopeGuard([subFD] () -> void {
                if (::close(subFD) < 0 && errno != EINTR) {
                    throw std::system_error(errno, std::system_category(), "close() failed");
                }
            });

            ioPoller_.createObject(subFD);
            scopeGuard.dismiss();
            return subFD;
        }
    }
}


int
Loop::connect(int fd, const sockaddr *name, socklen_t nameSize, int timeout)
{
    if (::connect(fd, name, nameSize) < 0) {
        if (errno == EINTR || errno == EINPROGRESS) {
            if (waitForFile(fd, IOCondition::Writable, std::chrono::milliseconds(timeout))) {
                int errorNumber;
                socklen_t errorNumberSize = sizeof(errorNumber);

                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &errorNumber, &errorNumberSize) < 0) {
                    throw std::system_error(errno, std::system_category(), "getsockopt() failed");
                }

                if (errorNumber == 0) {
                    return 0;
                } else {
                    errno = errorNumber;
                    return -1;
                }
            } else {
                errno = EAGAIN;
                return -1;
            }
        } else {
            return -1;
        }
    } else {
        return 0;
    }
}


ssize_t
Loop::recv(int fd, void *buffer, size_t bufferSize, int flags, int timeout)
{
    return readFile(fd, timeout, ::recv, buffer, bufferSize, flags);
}


ssize_t
Loop::send(int fd, const void *data, size_t dataSize, int flags, int timeout)
{
    return writeFile(fd, timeout, ::send, data, dataSize, flags);
}


ssize_t
Loop::recvfrom(int fd, void *buffer, size_t bufferSize, int flags, sockaddr *name
               , socklen_t *nameSize, int timeout)
{
    return readFile(fd, timeout, ::recvfrom, buffer, bufferSize, flags, name, nameSize);
}


ssize_t
Loop::sendto(int fd, const void *data, size_t dataSize, int flags, const sockaddr *name
             , socklen_t nameSize, int timeout)
{
    return writeFile(fd, timeout, ::sendto, data, dataSize, flags, name, nameSize);
}


ssize_t
Loop::recvmsg(int fd, msghdr *message, int flags, int timeout)
{
    return readFile(fd, timeout, ::recvmsg, message, flags);
}


ssize_t
Loop::sendmsg(int fd, const msghdr *message, int flags, int timeout)
{
    return writeFile(fd, timeout, ::sendmsg, message, flags);
}


int
Loop::close(int fd)
{
    ioPoller_.destroyObject(fd);
    return ::close(fd);
}


template <class Func, class ...Args>
ssize_t
Loop::readFile(int fd, int timeout, Func func, Args &&...args)
{
    for (;;) {
        ssize_t numberOfBytes = func(fd, std::forward<Args>(args)...);

        if (numberOfBytes < 0) {
            if (errno == EAGAIN) {
                if (!waitForFile(fd, IOCondition::Readable, std::chrono::milliseconds(timeout))) {
                    errno = EAGAIN;
                    return -1;
                }
            } else {
                if (errno != EINTR) {
                    return -1;
                }
            }
        } else {
            return numberOfBytes;
        }
    }
}


template <class Func, class ...Args>
ssize_t
Loop::writeFile(int fd, int timeout, Func func, Args &&...args)
{
    for (;;) {
        ssize_t numberOfBytes = func(fd, std::forward<Args>(args)...);

        if (numberOfBytes < 0) {
            if (errno == EAGAIN) {
                if (!waitForFile(fd, IOCondition::Writable, std::chrono::milliseconds(timeout))) {
                    errno = EAGAIN;
                    return -1;
                }
            } else {
                if (errno != EINTR) {
                    return -1;
                }
            }
        } else {
            return numberOfBytes;
        }
    }
}


bool
Loop::waitForFile(int fd, IOCondition ioCondition, std::chrono::milliseconds timeout)
{
    if (timeout.count() < 0) {
        struct {
            MyIOWatcher myIOWatcher;
            IOPoller *ioPoller;
            void *fiberHandle;
            Scheduler *scheduler;
        } context;

        context.myIOWatcher.callback = [&context] () -> void {
            context.ioPoller->removeWatcher(&context.myIOWatcher);
            context.scheduler->resumeFiber(context.fiberHandle);
        };

        (context.ioPoller = &ioPoller_)->addWatcher(&context.myIOWatcher, fd, ioCondition);

        auto scopeGuard = MakeScopeGuard([&context] () -> void {
            context.ioPoller->removeWatcher(&context.myIOWatcher);
        });

        (context.scheduler = &scheduler_)->suspendFiber(context.fiberHandle
                                                        = scheduler_.getCurrentFiber());
        scopeGuard.dismiss();
        return true;
    } else if (timeout.count() == 0) {
        return false;
    } else {
        struct {
            MyIOWatcher myIOWatcher;
            IOPoller *ioPoller;
            MyIOTimer myIOTimer;
            IOClock *ioClock;
            void *fiberHandle;
            Scheduler *scheduler;
            bool ok;
        } context;

        context.myIOWatcher.callback = [&context] () -> void {
            context.ioPoller->removeWatcher(&context.myIOWatcher);
            context.ioClock->removeTimer(&context.myIOTimer);
            context.scheduler->resumeFiber(context.fiberHandle);
            context.ok = true;
        };

        (context.ioPoller = &ioPoller_)->addWatcher(&context.myIOWatcher, fd, ioCondition);

        auto scopeGuard1 = MakeScopeGuard([&context] () -> void {
            context.ioPoller->removeWatcher(&context.myIOWatcher);
        });

        context.myIOTimer.callback = [&context] () -> void {
            context.ioPoller->removeWatcher(&context.myIOWatcher);
            context.scheduler->resumeFiber(context.fiberHandle);
            context.ok = false;
        };

        (context.ioClock = &ioClock_)->addTimer(&context.myIOTimer, timeout);

        auto scopeGuard2 = MakeScopeGuard([&context] () -> void {
            context.ioClock->removeTimer(&context.myIOTimer);
        });

        (context.scheduler = &scheduler_)->suspendFiber(context.fiberHandle
                                                        = scheduler_.getCurrentFiber());
        scopeGuard1.dismiss();
        scopeGuard2.dismiss();
        return context.ok;
    }
}


void
Loop::setDelay(std::chrono::milliseconds duration)
{
    struct {
        MyIOTimer myIOTimer;
        IOClock *ioClock;
        void *fiberHandle;
        Scheduler *scheduler;
    } context;

    context.myIOTimer.callback = [&context] () -> void {
        context.scheduler->resumeFiber(context.fiberHandle);
    };

    (context.ioClock = &ioClock_)->addTimer(&context.myIOTimer, duration);

    auto scopeGuard = MakeScopeGuard([&context] () -> void {
        context.ioClock->removeTimer(&context.myIOTimer);
    });

    (context.scheduler = &scheduler_)->suspendFiber(context.fiberHandle
                                                    = scheduler_.getCurrentFiber());
    scopeGuard.dismiss();
}


namespace {

bool
SetBlocking(int fd, bool blocking)
{
    int flags = fcntl(fd, F_GETFL);

    if (flags < 0) {
        throw std::system_error(errno, std::system_category(), "fcntl() failed");
    }

    if ((flags & O_NONBLOCK) == O_NONBLOCK) {
        if (blocking) {
            flags &= ~O_NONBLOCK;
        } else {
            return false;
        }
    } else {
        if (blocking) {
            return false;
        } else {
            flags |= O_NONBLOCK;
        }
    }

    if (fcntl(fd, F_SETFL, flags) < 0) {
        throw std::system_error(errno, std::system_category(), "fcntl() failed");
    }

    return true;
}

} // namespace

} // namespace siren
