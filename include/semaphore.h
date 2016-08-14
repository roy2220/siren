#pragma once


#include <cstdint>

#include "list.h"


namespace siren {

class Scheduler;


namespace detail {

struct SemaphoreWaiter
  : ListNode
{
    void *fiberHandle;
};

}


class Semaphore final
{
public:
    inline explicit Semaphore(Scheduler *, std::intmax_t, std::intmax_t, std::intmax_t) noexcept;
    inline Semaphore(Semaphore &&) noexcept;
    inline ~Semaphore();
    inline Semaphore &operator=(Semaphore &&) noexcept;

    inline void reset() noexcept;
    inline void up() noexcept;
    inline void down() noexcept;
    inline bool tryUp() noexcept;
    inline bool tryDown() noexcept;

private:
    typedef detail::SemaphoreWaiter Waiter;

    Scheduler *const scheduler_;
    List upWaiterList_;
    List downWaiterList_;
    const std::intmax_t minValue_;
    const std::intmax_t maxValue_;
    const std::intmax_t initialValue_;
    std::intmax_t value_;

    inline void initialize() noexcept;
    inline void move(Semaphore *) noexcept;
#ifndef NDEBUG
    inline bool isWaited() const noexcept;
#endif
};

}


/*
 * #include "semaphore-inl.h"
 */


#include <cassert>

#include "scheduler.h"


namespace siren {

Semaphore::Semaphore(Scheduler *scheduler, std::intmax_t minValue, std::intmax_t maxValue
                     , std::intmax_t initialValue) noexcept
  : scheduler_(scheduler),
    minValue_(minValue),
    maxValue_(maxValue),
    initialValue_(initialValue)
{
    assert(scheduler != nullptr);
    assert(initialValue >= minValue);
    assert(initialValue <= maxValue);
    initialize();
}


Semaphore::Semaphore(Semaphore &&other) noexcept
  : scheduler_(other.scheduler_),
    minValue_(other.minValue_),
    maxValue_(other.maxValue_),
    initialValue_(other.initialValue_)
{
    assert(!other.isWaited());
    other.move(this);
}


Semaphore::~Semaphore()
{
    assert(!isWaited());
}


Semaphore &
Semaphore::operator=(Semaphore &&other) noexcept
{
    if (&other != this) {
        assert(!isWaited());
        assert(!other.isWaited());
        assert(minValue_ == other.minValue_);
        assert(maxValue_ == other.maxValue_);
        assert(initialValue_ == other.initialValue_);
        other.move(this);
    }

    return *this;
}


void
Semaphore::initialize() noexcept
{
    value_ = initialValue_;
}


void
Semaphore::move(Semaphore *other) noexcept
{
    other->value_ = value_;
    initialize();
}


void
Semaphore::reset() noexcept
{
    assert(!isWaited());
    initialize();
}


#ifndef NDEBUG
bool
Semaphore::isWaited() const noexcept
{
    return !upWaiterList_.isEmpty() || !downWaiterList_.isEmpty();
}
#endif


void
Semaphore::up() noexcept
{
    if (value_ == maxValue_) {
        {
            Waiter waiter;
            upWaiterList_.addTail(&waiter);
            scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
            waiter.remove();
        }

        if (++value_ < maxValue_ && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getHead());
            scheduler_->resumeFiber(waiter->fiberHandle);
        }
    } else {
        if (++value_ == maxValue_ && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getHead());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }
    }

    if (value_ == minValue_ + 1 && !downWaiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(downWaiterList_.getHead());
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


void
Semaphore::down() noexcept
{
    if (value_ == minValue_) {
        {
            Waiter waiter;
            downWaiterList_.addTail(&waiter);
            scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
            waiter.remove();
        }

        if (--value_ > minValue_ && !downWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(downWaiterList_.getHead());
            scheduler_->resumeFiber(waiter->fiberHandle);
        }
    } else {
        if (--value_ == minValue_ && !downWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(downWaiterList_.getHead());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }
    }

    if (value_ == maxValue_ - 1 && !upWaiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(upWaiterList_.getHead());
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


bool
Semaphore::tryUp() noexcept
{
    if (value_ == maxValue_) {
        return false;
    } else {
        if (++value_ == maxValue_ && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getHead());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }

        if (value_ == minValue_ + 1 && !downWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(downWaiterList_.getHead());
            scheduler_->resumeFiber(waiter->fiberHandle);
        }

        return true;
    }
}


bool
Semaphore::tryDown() noexcept
{
    if (value_ == minValue_) {
        return false;
    } else {
        if (--value_ == minValue_ && !downWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(downWaiterList_.getHead());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }

        if (value_ == maxValue_ - 1 && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getHead());
            scheduler_->resumeFiber(waiter->fiberHandle);
        }

        return true;
    }
}

}
