#pragma once


#include "semaphore.h"


namespace siren {

class Scheduler;


class Mutex final
{
public:
    inline explicit Mutex(Scheduler *) noexcept;

    inline void reset() noexcept;
    inline void lock();
    inline void unlock();
    inline bool tryLock() noexcept;
    inline bool tryUnlock() noexcept;

    Mutex(Mutex &&) noexcept = default;
    Mutex &operator=(Mutex &&) noexcept = default;

private:
    Semaphore semaphore_;
};

}


/*
 * #include "mutex-inl.h"
 */


namespace siren {

Mutex::Mutex(Scheduler *scheduler) noexcept
  : semaphore_(scheduler, 0, 0, 1)
{
}


void
Mutex::reset() noexcept
{
    semaphore_.reset();
}


void
Mutex::lock()
{
    semaphore_.up();
}


void
Mutex::unlock()
{
    semaphore_.down();
}


bool
Mutex::tryLock() noexcept
{
    return semaphore_.tryDown();
}


bool
Mutex::tryUnlock() noexcept
{
    return semaphore_.tryUp();
}

}
