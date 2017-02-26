#pragma once


#include <cstddef>

#include <poll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "event.h"
#include "io_clock.h"
#include "io_poller.h"
#include "mutex.h"
#include "scheduler.h"
#include "semaphore.h"


namespace siren {

namespace detail { struct FileOptions; }


class Loop final
{
public:
    inline void *createFiber(const std::function<void ()> &, std::size_t = 0, bool = false);
    inline void *createFiber(std::function<void ()> &&, std::size_t = 0, bool = false);
    inline void interruptFiber(void *);
    inline void *getCurrentFiber() noexcept;
    inline void yieldToScheduler();
    inline Event makeEvent() noexcept;
    inline Mutex makeMutex() noexcept;
    inline Semaphore makeSemaphore(std::intmax_t = 0, std::intmax_t = 0
                                   , std::intmax_t = std::numeric_limits<std::intmax_t>::max())
        noexcept;
    inline int usleep(useconds_t);
    inline int pipe(int [2]);
    inline int accept(int, sockaddr *, socklen_t *);

    Loop(Loop &&) noexcept = default;
    Loop &operator=(Loop &&) noexcept = default;

    explicit Loop(std::size_t = 0);

    void run();
    void registerFD(int);
    void unregisterFD(int) noexcept;
    int open(const char *, int, mode_t = 0);
    int fcntl(int, int, int = 0) noexcept;
    int pipe2(int [2], int);
    ssize_t read(int, void *, size_t);
    ssize_t write(int, const void *, size_t);
    ssize_t readv(int, const iovec *, int);
    ssize_t writev(int, const iovec *, int);
    int socket(int, int, int);
    int getsockopt(int, int, int, void *, socklen_t *) const noexcept;
    int setsockopt(int, int, int, const void *, socklen_t) noexcept;
    int accept4(int, sockaddr *, socklen_t *, int);
    int connect(int, const sockaddr *, socklen_t);
    ssize_t recv(int, void *, size_t, int);
    ssize_t send(int, const void *, size_t, int);
    ssize_t recvfrom(int, void *, size_t, int, sockaddr *, socklen_t *);
    ssize_t sendto(int, const void *, size_t, int, const sockaddr *, socklen_t);
    int close(int) noexcept;
    int poll(pollfd *, nfds_t, int);

private:
    typedef detail::FileOptions FileOptions;

    IOPoller ioPoller_;
    IOClock ioClock_;
    Scheduler scheduler_;

    const FileOptions *getFileOptions(int) const noexcept;
    FileOptions *getFileOptions(int) noexcept;
    void createIOContext(int, bool, bool, long = -1, long = -1);
    void destroyIOContext(int) noexcept;
    bool ioContextExists(int) const;
    long getEffectiveReadTimeout(int) const noexcept;
    long getEffectiveWriteTimeout(int) const noexcept;
    bool waitForFile(int, IOCondition, IOCondition *, std::chrono::milliseconds);
    void setDelay(std::chrono::milliseconds);

    template <class Func, class ...Args>
    ssize_t readFile(int, long, Func, Args &&...);

    template <class Func, class ...Args>
    ssize_t writeFile(int, long, Func, Args &&...);
};

} // namespace siren


/*
 * #include "loop-inl.h"
 */


#include <utility>

#include "assert.h"


namespace siren {

void *
Loop::createFiber(const std::function<void ()> &procedure, std::size_t fiberSize
                  , bool fiberIsBackground)
{
    SIREN_ASSERT(procedure != nullptr);
    return scheduler_.createFiber(procedure, fiberSize, fiberIsBackground);
}


void *
Loop::createFiber(std::function<void ()> &&procedure, std::size_t fiberSize
                  , bool fiberIsBackground)
{
    SIREN_ASSERT(procedure != nullptr);
    return scheduler_.createFiber(std::move(procedure), fiberSize, fiberIsBackground);
}


void
Loop::interruptFiber(void *fiberHandle)
{
    return scheduler_.interruptFiber(fiberHandle);
}


void *
Loop::getCurrentFiber() noexcept
{
    return scheduler_.getCurrentFiber();
}


void
Loop::yieldToScheduler()
{
    scheduler_.yieldTo();
}


Event
Loop::makeEvent() noexcept
{
    return Event(&scheduler_);
}


Mutex
Loop::makeMutex() noexcept
{
    return Mutex(&scheduler_);
}


Semaphore
Loop::makeSemaphore(std::intmax_t initialValue, std::intmax_t minValue
                    , std::intmax_t maxValue) noexcept
{
    return Semaphore(&scheduler_, initialValue, minValue, maxValue);
}


int
Loop::usleep(useconds_t duration)
{
    setDelay(std::chrono::milliseconds(duration / 1000));
    return 0;
}


int
Loop::pipe(int fds[2])
{
    return pipe2(fds, 0);
}


int
Loop::accept(int fd, sockaddr *name, socklen_t *nameSize)
{
    return accept4(fd, name, nameSize, 0);
}

} // namespace siren
