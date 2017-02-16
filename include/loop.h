#pragma once


#include <cstddef>

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
    inline unsigned int sleep(unsigned int);
    inline int usleep(useconds_t);
    inline int pipe(int [2]);
    inline int accept(int, sockaddr *, socklen_t *, int = -1);

    Loop(Loop &&) noexcept = default;
    Loop &operator=(Loop &&) noexcept = default;

    explicit Loop(std::size_t = 0);

    void run();
    void registerFD(int);
    void unregisterFD(int);
    int open(const char *, int, mode_t);
    int pipe2(int [2], int);
    ssize_t read(int, void *, size_t, int = -1);
    ssize_t write(int, const void *, size_t, int = -1);
    ssize_t readv(int, const iovec *, int, int = -1);
    ssize_t writev(int, const iovec *, int, int = -1);
    int socket(int, int, int);
    int accept4(int, sockaddr *, socklen_t *, int, int = -1);
    int connect(int, const sockaddr *, socklen_t, int = -1);
    ssize_t recv(int, void *, size_t, int, int = -1);
    ssize_t send(int, const void *, size_t, int, int = -1);
    ssize_t recvfrom(int, void *, size_t, int, sockaddr *, socklen_t *, int = -1);
    ssize_t sendto(int, const void *, size_t, int, const sockaddr *, socklen_t, int = -1);
    ssize_t recvmsg(int, msghdr *, int, int = -1);
    ssize_t sendmsg(int, const msghdr *, int, int = -1);
    int close(int);

private:
    IOPoller ioPoller_;
    IOClock ioClock_;
    Scheduler scheduler_;

    bool waitForFile(int, IOCondition, std::chrono::milliseconds);
    void setDelay(std::chrono::milliseconds);

    template <class Func, class ...Args>
    ssize_t readFile(int, int, Func, Args &&...);

    template <class Func, class ...Args>
    ssize_t writeFile(int, int, Func, Args &&...);
};

} // namespace siren


/*
 * #include "loop-inl.h"
 */


#include <cassert>
#include <utility>


namespace siren {

void *
Loop::createFiber(const std::function<void ()> &procedure, std::size_t fiberSize
                  , bool fiberIsBackground)
{
    assert(procedure != nullptr);
    return scheduler_.createFiber(procedure, fiberSize, fiberIsBackground);
}


void *
Loop::createFiber(std::function<void ()> &&procedure, std::size_t fiberSize
                  , bool fiberIsBackground)
{
    assert(procedure != nullptr);
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
Loop::makeSemaphore(std::intmax_t initialValue, std::intmax_t minValue, std::intmax_t maxValue)
    noexcept
{
    return Semaphore(&scheduler_, initialValue, minValue, maxValue);
}


unsigned int
Loop::sleep(unsigned int duration)
{
    setDelay(std::chrono::milliseconds(duration) * 1000);
    return 0;
}


int
Loop::usleep(useconds_t duration)
{
    setDelay(std::chrono::milliseconds(duration) / 1000);
    return 0;
}


int
Loop::pipe(int fds[2])
{
    return pipe2(fds, 0);
}


int
Loop::accept(int fd, sockaddr *name, socklen_t *nameSize, int timeout)
{
    return accept4(fd, name, nameSize, 0, timeout);
}

} // namespace siren
