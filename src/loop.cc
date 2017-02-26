#include "loop.h"

#include <cerrno>
#include <cstdio>
#include <functional>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>

#include "config.h"
#include "macros.h"
#include "scope_guard.h"


namespace siren {

#ifdef SIREN_WITH_DEBUG
#  define CHECK_FD(FD) SIREN_ASSERT(ioContextExists((FD)))
#else
#  define CHECK_FD(FD)                \
    do {                              \
        if (!ioContextExists((FD))) { \
            errno = EBADF;            \
            return -1;                \
        }                             \
    } while (false)
#endif


namespace detail {

struct FileOptions
{
    bool isSocket: 1;
    bool blocking: 1;
    long readTimeout;
    long writeTimeout;
};

} // namespace detail


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
long TimeToTimeout(timeval);
timeval TimeoutToTime(long);

} // namespace


Loop::Loop(std::size_t defaultFiberSize)
  : ioPoller_(alignof(FileOptions), sizeof(FileOptions)),
    scheduler_(defaultFiberSize)
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
            return;
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
    bool isSocket;

    {
        struct stat status;
        fstat(fd, &status);
        isSocket = S_ISSOCK(status.st_mode);
    }

    bool blocking = SetBlocking(fd, false);

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        if (blocking) {
            SetBlocking(fd, true);
        }
    });

    if (isSocket) {
        long readTimeout;
        long writeTimeout;

        {
            timeval time;
            socklen_t timeSize = sizeof(time);

            if (::getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &time, &timeSize) < 0) {
                throw std::system_error(errno, std::system_category(), "getsockopt() failed");
            }

            readTimeout = TimeToTimeout(time);
        }

        {
            timeval time;
            socklen_t timeSize = sizeof(time);

            if (::getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &time, &timeSize) < 0) {
                throw std::system_error(errno, std::system_category(), "getsockopt() failed");
            }

            writeTimeout = TimeToTimeout(time);
        }

        createIOContext(fd, isSocket, blocking, readTimeout, writeTimeout);
    } else {
        createIOContext(fd, isSocket, blocking);
    }

    scopeGuard.dismiss();
}


void
Loop::unregisterFD(int fd) noexcept
{
    FileOptions *fileOptions = getFileOptions(fd);

    if (fileOptions->blocking) {
        SetBlocking(fd, true);
    }

    if (fileOptions->isSocket)
    {
        {
            timeval time = TimeoutToTime(fileOptions->readTimeout);

            if (::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time)) < 0) {
                std::perror("setsockopt() failed");
                std::terminate();
            }
        }

        {
            timeval time = TimeoutToTime(fileOptions->writeTimeout);

            if (::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &time, sizeof(time)) < 0) {
                std::perror("setsockopt() failed");
                std::terminate();
            }
        }
    }

    destroyIOContext(fd);
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
            auto scopeGuard = MakeScopeGuard([&] () -> void {
                if (::close(fd) < 0 && errno != EINTR) {
                    std::perror("close() failed");
                    std::terminate();
                }
            });

            bool blocking = (mode & O_NONBLOCK) == 0;
            createIOContext(fd, false, blocking);
            scopeGuard.dismiss();
            return fd;
        }
    }
}



int
Loop::fcntl(int fd, int command, int argument) noexcept
{
    CHECK_FD(fd);

    if (command == F_GETFL) {
        SIREN_UNUSED(argument);
        int flags = ::fcntl(fd, F_GETFL);

        if (flags < 0) {
            return -1;
        } else {
            return (flags & ~O_NONBLOCK) | (getFileOptions(fd)->blocking ? 0 : O_NONBLOCK);
        }
    } else if (command == F_SETFL){
        int flags = argument;

        if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            return -1;
        } else {
            getFileOptions(fd)->blocking = (flags & O_NONBLOCK) == 0;
            return 0;
        }
    } else {
        return ::fcntl(fd, command, argument);
    }
}


int
Loop::pipe2(int fds[2], int flags)
{
    if (::pipe2(fds, flags | O_NONBLOCK) < 0) {
        return -1;
    } else {
        auto scopeGuard1 = MakeScopeGuard([&] () -> void {
            if (::close(fds[0]) < 0 && errno != EINTR) {
                std::perror("close() failed");
                std::terminate();
            }

            if (::close(fds[1]) < 0 && errno != EINTR) {
                std::perror("close() failed");
                std::terminate();
            }
        });

        bool blocking = (flags & O_NONBLOCK) == 0;
        createIOContext(fds[0], false, blocking);

        auto scopeGuard2 = MakeScopeGuard([&] () -> void {
            destroyIOContext(fds[0]);
        });

        createIOContext(fds[1], false, blocking);
        scopeGuard1.dismiss();
        scopeGuard2.dismiss();
        return 0;
    }
}


ssize_t
Loop::read(int fd, void *buffer, size_t bufferSize)
{
    CHECK_FD(fd);
    return readFile(fd, getEffectiveReadTimeout(fd), ::read, buffer, bufferSize);
}


ssize_t
Loop::write(int fd, const void *data, size_t dataSize)
{
    CHECK_FD(fd);
    return writeFile(fd, getEffectiveWriteTimeout(fd), ::write, data, dataSize);
}


ssize_t
Loop::readv(int fd, const iovec *vector, int vectorLength)
{
    CHECK_FD(fd);
    return readFile(fd, getEffectiveReadTimeout(fd), ::readv, vector, vectorLength);
}


ssize_t
Loop::writev(int fd, const iovec *vector, int vectorLength)
{
    CHECK_FD(fd);
    return writeFile(fd, getEffectiveWriteTimeout(fd), ::writev, vector, vectorLength);
}


int
Loop::socket(int domain, int type, int protocol)
{
    int fd = ::socket(domain, type | SOCK_NONBLOCK, protocol);

    if (fd < 0) {
        return -1;
    } else {
        auto scopeGuard = MakeScopeGuard([&] () -> void {
            if (::close(fd) < 0 && errno != EINTR) {
                std::perror("close() failed");
                std::terminate();
            }
        });

        bool blocking = (type & SOCK_NONBLOCK) == 0;
        createIOContext(fd, true, blocking);
        scopeGuard.dismiss();
        return fd;
    }
}


int
Loop::getsockopt(int fd, int level, int optionType, void *optionValue
                 , socklen_t *optionValueSize) const noexcept
{
    CHECK_FD(fd);

    if (level == SOL_SOCKET && (optionType == SO_RCVTIMEO || optionType == SO_SNDTIMEO)) {
        const FileOptions *fileOptions = getFileOptions(fd);

        if (fileOptions->isSocket) {
            if (optionValueSize == nullptr || *optionValueSize < sizeof(timeval)) {
                errno = EINVAL;
                return -1;
            } else {
                auto time = static_cast<timeval *>(optionValue);

                if (optionType == SO_RCVTIMEO) {
                    *time = TimeoutToTime(fileOptions->readTimeout);
                } else {
                    *time = TimeoutToTime(fileOptions->writeTimeout);
                }

                return 0;
            }
        } else {
            errno = ENOTSOCK;
            return -1;
        }
    } else {
        return ::getsockopt(fd, level, optionType, optionValue, optionValueSize);
    }
}


int
Loop::setsockopt(int fd, int level, int optionType, const void *optionValue
                 , socklen_t optionValueSize) noexcept
{
    CHECK_FD(fd);

    if (level == SOL_SOCKET && (optionType == SO_RCVTIMEO || optionType == SO_SNDTIMEO)) {
        FileOptions *fileOptions = getFileOptions(fd);

        if (fileOptions->isSocket) {
            if (optionValueSize < sizeof(timeval)) {
                errno = EINVAL;
                return -1;
            } else {
                auto time = static_cast<const timeval *>(optionValue);

                if (optionType == SO_RCVTIMEO) {
                    fileOptions->readTimeout = TimeToTimeout(*time);
                } else {
                    fileOptions->writeTimeout = TimeToTimeout(*time);
                }

                return 0;
            }
        } else {
            errno = ENOTSOCK;
            return -1;
        }
    } else {
        return ::setsockopt(fd, level, optionType, optionValue, optionValueSize);
    }
}


int
Loop::accept4(int fd, sockaddr *name, socklen_t *nameSize, int flags)
{
    CHECK_FD(fd);

    for (;;) {
        int subFD = ::accept4(fd, name, nameSize, flags | SOCK_NONBLOCK);

        if (subFD < 0) {
            if (errno == EAGAIN) {
                if (!waitForFile(fd, IOCondition::In, nullptr
                                 , std::chrono::milliseconds(getEffectiveReadTimeout(fd)))) {
                    errno = EAGAIN;
                    return -1;
                }
            } else {
                if (errno != EINTR) {
                    return -1;
                }
            }
        } else {
            auto scopeGuard = MakeScopeGuard([&] () -> void {
                if (::close(subFD) < 0 && errno != EINTR) {
                    std::perror("close() failed");
                    std::terminate();
                }
            });

            bool blocking = (flags & SOCK_NONBLOCK) == 0;
            createIOContext(subFD, true, blocking);
            scopeGuard.dismiss();
            return subFD;
        }
    }
}


int
Loop::connect(int fd, const sockaddr *name, socklen_t nameSize)
{
    CHECK_FD(fd);

    if (::connect(fd, name, nameSize) < 0) {
        if (errno == EINTR || errno == EINPROGRESS) {
            if (waitForFile(fd, IOCondition::Out, nullptr
                            , std::chrono::milliseconds(getEffectiveWriteTimeout(fd)))) {
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
                errno = EINPROGRESS;
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
Loop::recv(int fd, void *buffer, size_t bufferSize, int flags)
{
    CHECK_FD(fd);
    long timeout;

    if ((flags & MSG_DONTWAIT) == MSG_DONTWAIT) {
        flags &= ~MSG_DONTWAIT;
        timeout = 0;
    } else {
        timeout = getEffectiveReadTimeout(fd);
    }

    if ((flags & MSG_WAITALL) == MSG_WAITALL) {
        flags &= ~MSG_WAITALL;
        size_t byteCount = 0;

        for (;;) {
            ssize_t numberOfBytes = readFile(fd, timeout, ::recv
                                             , static_cast<char *>(buffer) + byteCount
                                             , bufferSize - byteCount, flags);

            if (numberOfBytes < 0) {
                return byteCount == 0 ? -1 : static_cast<ssize_t>(byteCount);
            } else if (numberOfBytes == 0) {
                return byteCount;
            } else {
                byteCount += numberOfBytes;

                if (byteCount == bufferSize) {
                    return byteCount;
                }
            }
        }
    } else {
        return readFile(fd, timeout, ::recv, buffer, bufferSize, flags);
    }
}


ssize_t
Loop::send(int fd, const void *data, size_t dataSize, int flags)
{
    CHECK_FD(fd);
    long timeout;

    if ((flags & MSG_DONTWAIT) == MSG_DONTWAIT) {
        flags &= ~MSG_DONTWAIT;
        timeout = 0;
    } else {
        timeout = getEffectiveWriteTimeout(fd);
    }

    return writeFile(fd, timeout, ::send, data, dataSize, flags);
}


ssize_t
Loop::recvfrom(int fd, void *buffer, size_t bufferSize, int flags, sockaddr *name
               , socklen_t *nameSize)
{
    CHECK_FD(fd);
    long timeout;

    if ((flags & MSG_DONTWAIT) == MSG_DONTWAIT) {
        flags &= ~MSG_DONTWAIT;
        timeout = 0;
    } else {
        timeout = getEffectiveReadTimeout(fd);
    }

    if ((flags & MSG_WAITALL) == MSG_WAITALL) {
        flags &= ~MSG_WAITALL;
        size_t byteCount = 0;

        for (;;) {
            ssize_t numberOfBytes = readFile(fd, timeout, ::recvfrom
                                             , static_cast<char *>(buffer) + byteCount
                                             , bufferSize - byteCount, flags, name, nameSize);

            if (numberOfBytes < 0) {
                return byteCount == 0 ? -1 : static_cast<ssize_t>(byteCount);
            } else if (numberOfBytes == 0) {
                return byteCount;
            } else {
                byteCount += numberOfBytes;

                if (byteCount == bufferSize) {
                    return byteCount;
                }
            }
        }
    } else {
        return readFile(fd, timeout, ::recvfrom, buffer, bufferSize, flags, name, nameSize);
    }
}


ssize_t
Loop::sendto(int fd, const void *data, size_t dataSize, int flags, const sockaddr *name
             , socklen_t nameSize)
{
    CHECK_FD(fd);
    long timeout;

    if ((flags & MSG_DONTWAIT) == MSG_DONTWAIT) {
        flags &= ~MSG_DONTWAIT;
        timeout = 0;
    } else {
        timeout = getEffectiveWriteTimeout(fd);
    }

    return writeFile(fd, timeout, ::sendto, data, dataSize, flags, name, nameSize);
}


int
Loop::close(int fd) noexcept
{
    CHECK_FD(fd);
    destroyIOContext(fd);
    return ::close(fd);
}


int
Loop::poll(pollfd *pollFDs, nfds_t numberOfPollFDs, int timeout)
{
    if (numberOfPollFDs == 0) {
        setDelay(std::chrono::milliseconds(timeout));
        return 0;
    } else if (numberOfPollFDs == 1) {
        if (pollFDs == nullptr) {
            errno = EFAULT;
            return -1;
        } else {
            pollfd *pollFD = &pollFDs[0];

            if (ioContextExists(pollFD->fd)) {
                IOCondition ioConditions = IOCondition::No;

                if ((pollFD->events & POLLIN) == POLLIN) {
                    ioConditions |= IOCondition::In;
                }

                if ((pollFD->events & POLLOUT) == POLLOUT) {
                    ioConditions |= IOCondition::Out;
                }

                if ((pollFD->events & POLLRDHUP) == POLLRDHUP) {
                    ioConditions |= IOCondition::RdHup;
                }

                if ((pollFD->events & POLLPRI) == POLLPRI) {
                    ioConditions |= IOCondition::Pri;
                }

                IOCondition readyIOConditions;

                if (waitForFile(pollFD->fd, ioConditions, &readyIOConditions
                                , std::chrono::milliseconds(timeout))) {
                    pollFD->revents = 0;

                    if ((readyIOConditions & IOCondition::In) == IOCondition::In) {
                        pollFD->revents |= POLLIN;
                    }

                    if ((readyIOConditions & IOCondition::Out) == IOCondition::Out) {
                        pollFD->revents |= POLLOUT;
                    }

                    if ((readyIOConditions & IOCondition::RdHup) == IOCondition::RdHup) {
                        pollFD->revents |= POLLRDHUP;
                    }

                    if ((readyIOConditions & IOCondition::Pri) == IOCondition::Pri) {
                        pollFD->revents |= POLLPRI;
                    }

                    if ((readyIOConditions & IOCondition::Err) == IOCondition::Err) {
                        pollFD->revents |= POLLERR;
                    }

                    if ((readyIOConditions & IOCondition::Hup) == IOCondition::Hup) {
                        pollFD->revents |= POLLHUP;
                    }

                    return 1;
                } else {
                    return 0;
                }
            } else {
#ifdef SIREN_WITH_DEBUG
                SIREN_ASSERT(false);
#else
                pollFD->revents = POLLNVAL;
                return 1;
#endif
            }
        }
    } else {
        errno = ENOSYS;
        return -1;
    }
}



template <class Func, class ...Args>
ssize_t
Loop::readFile(int fd, long timeout, Func func, Args &&...args)
{
    for (;;) {
        ssize_t numberOfBytes = func(fd, std::forward<Args>(args)...);

        if (numberOfBytes < 0) {
            if (errno == EAGAIN) {
                if (!waitForFile(fd, IOCondition::In, nullptr
                                 , std::chrono::milliseconds(timeout))) {
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
Loop::writeFile(int fd, long timeout, Func func, Args &&...args)
{
    for (;;) {
        ssize_t numberOfBytes = func(fd, std::forward<Args>(args)...);

        if (numberOfBytes < 0) {
            if (errno == EAGAIN) {
                if (!waitForFile(fd, IOCondition::Out, nullptr
                                 , std::chrono::milliseconds(timeout))) {
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


const detail::FileOptions *
Loop::getFileOptions(int fd) const noexcept
{
    return static_cast<const FileOptions *>(ioPoller_.getContextTag(fd));
}


detail::FileOptions *
Loop::getFileOptions(int fd) noexcept
{
    return static_cast<FileOptions *>(ioPoller_.getContextTag(fd));
}


void
Loop::createIOContext(int fd, bool isSocket, bool blocking, long readTimeout, long writeTimeout)
{
    ioPoller_.createContext(fd);
    FileOptions *fileOptions = getFileOptions(fd);
    fileOptions->isSocket = isSocket;
    fileOptions->blocking = blocking;
    fileOptions->readTimeout = readTimeout;
    fileOptions->writeTimeout = writeTimeout;
}


void
Loop::destroyIOContext(int fd) noexcept
{
    ioPoller_.destroyContext(fd);
}


bool
Loop::ioContextExists(int fd) const
{
    return ioPoller_.contextExists(fd);
}


long
Loop::getEffectiveReadTimeout(int fd) const noexcept
{
    const FileOptions *fileOptions = getFileOptions(fd);
    return fileOptions->blocking ? fileOptions->readTimeout : 0;
}


long
Loop::getEffectiveWriteTimeout(int fd) const noexcept
{
    const FileOptions *fileOptions = getFileOptions(fd);
    return fileOptions->blocking ? fileOptions->writeTimeout : 0;
}


bool
Loop::waitForFile(int fd, IOCondition ioConditions, IOCondition *readyIOConditions
                  , std::chrono::milliseconds timeout)
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

        (context.ioPoller = &ioPoller_)->addWatcher(&context.myIOWatcher, fd, ioConditions);

        auto scopeGuard = MakeScopeGuard([&] () -> void {
            context.ioPoller->removeWatcher(&context.myIOWatcher);
        });

        (context.scheduler = &scheduler_)->suspendFiber(context.fiberHandle
                                                        = scheduler_.getCurrentFiber());

        if (readyIOConditions != nullptr) {
            *readyIOConditions = context.myIOWatcher.getReadyConditions();
        }

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

        (context.ioPoller = &ioPoller_)->addWatcher(&context.myIOWatcher, fd, ioConditions);

        auto scopeGuard1 = MakeScopeGuard([&] () -> void {
            context.ioPoller->removeWatcher(&context.myIOWatcher);
        });

        context.myIOTimer.callback = [&context] () -> void {
            context.ioPoller->removeWatcher(&context.myIOWatcher);
            context.scheduler->resumeFiber(context.fiberHandle);
            context.ok = false;
        };

        (context.ioClock = &ioClock_)->addTimer(&context.myIOTimer, timeout);

        auto scopeGuard2 = MakeScopeGuard([&] () -> void {
            context.ioClock->removeTimer(&context.myIOTimer);
        });

        (context.scheduler = &scheduler_)->suspendFiber(context.fiberHandle
                                                        = scheduler_.getCurrentFiber());

        if (context.ok && readyIOConditions != nullptr) {
            *readyIOConditions = context.myIOWatcher.getReadyConditions();
        }

        scopeGuard1.dismiss();
        scopeGuard2.dismiss();
        return context.ok;
    }
}


void
Loop::setDelay(std::chrono::milliseconds duration)
{
    if (duration.count() < 0) {
        scheduler_.suspendFiber(scheduler_.getCurrentFiber());
    } else {
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

        auto scopeGuard = MakeScopeGuard([&] () -> void {
            context.ioClock->removeTimer(&context.myIOTimer);
        });

        (context.scheduler = &scheduler_)->suspendFiber(context.fiberHandle
                                                        = scheduler_.getCurrentFiber());
        scopeGuard.dismiss();
    }
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
            return true;
        } else {
            flags |= O_NONBLOCK;
        }
    }

    if (fcntl(fd, F_SETFL, flags) < 0) {
        throw std::system_error(errno, std::system_category(), "fcntl() failed");
    }

    return !blocking;
}


long
TimeToTimeout(timeval time)
{
    long timeout;

    if (time.tv_sec == 0 && time.tv_usec == 0) {
        timeout = -1;
    } else {
        timeout = time.tv_sec * 1000 + time.tv_usec / 1000;
    }

    return timeout;
}


timeval
TimeoutToTime(long timeout)
{
    timeval time;

    if (timeout < 0) {
        time.tv_sec = 0;
        time.tv_usec = 0;
    } else {
        time.tv_sec = timeout / 1000;
        time.tv_usec = (timeout % 1000) * 1000;
    }

    return time;
}

} // namespace

} // namespace siren
