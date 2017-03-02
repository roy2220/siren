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

private:
    Semaphore semaphore_;
};

} // namespace siren


/*
 * #include "mutex-inl.h"
 */


namespace siren {

Mutex::Mutex(Scheduler *scheduler) noexcept
  : semaphore_(scheduler, 1, 0, 1)
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
    semaphore_.down();
}


void
Mutex::unlock()
{
    semaphore_.up();
}


bool
Mutex::tryLock() noexcept
{
    return semaphore_.tryUp();
}


bool
Mutex::tryUnlock() noexcept
{
    return semaphore_.tryDown();
}

} // namespace siren
