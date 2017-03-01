#include "semaphore.h"

#include "assert.h"
#include "config.h"
#include "scheduler.h"


namespace siren {

namespace detail {

struct SemaphoreWaiter
  : ListNode
{
    void *fiberHandle;
};

} // namespace detail


Semaphore::Semaphore(Scheduler *scheduler, std::intmax_t initialValue, std::intmax_t minValue
                     , std::intmax_t maxValue) noexcept
  : scheduler_(scheduler),
    initialValue_(initialValue),
    minValue_(minValue),
    maxValue_(maxValue)
{
    SIREN_ASSERT(scheduler != nullptr);
    SIREN_ASSERT(initialValue >= minValue);
    SIREN_ASSERT(initialValue <= maxValue);
    initialize();
}


Semaphore::Semaphore(Semaphore &&other) noexcept
  : scheduler_(other.scheduler_),
    initialValue_(other.initialValue_),
    minValue_(other.minValue_),
    maxValue_(other.maxValue_)
{
    SIREN_ASSERT(!other.isWaited());
    other.move(this);
}


Semaphore::~Semaphore()
{
    SIREN_ASSERT(!isWaited());
}


Semaphore &
Semaphore::operator=(Semaphore &&other) noexcept
{
    if (&other != this) {
        SIREN_ASSERT(!isWaited());
        SIREN_ASSERT(!other.isWaited());
        SIREN_ASSERT(initialValue_ == other.initialValue_);
        SIREN_ASSERT(minValue_ == other.minValue_);
        SIREN_ASSERT(maxValue_ == other.maxValue_);
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
    if (value_ != initialValue_) {
        if (value_ == minValue_) {
            downWaiterWakes();
        } else if (value_ == maxValue_) {
            upWaiterWakes();
        } else if (initialValue_ == minValue_) {
            downWaiterSleeps();
        } else if (initialValue_ == maxValue_) {
            upWaiterSleeps();
        }

        value_ = initialValue_;
    }
}


#ifdef SIREN_WITH_DEBUG
bool
Semaphore::isWaited() const noexcept
{
    return !upWaiterList_.isEmpty() || !downWaiterList_.isEmpty();
}
#endif


void
Semaphore::down()
{
    if (value_ == minValue_) {
        {
            Waiter waiter;
            downWaiterList_.appendNode(&waiter);

            auto scopeGuard = MakeScopeGuard([&] () -> void {
                waiter.remove();
            });

            scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
        }

        if (--value_ > minValue_) {
            downWaiterWakes();
        }
    } else {
        if (--value_ == minValue_) {
            downWaiterSleeps();
        }
    }

    if (value_ == maxValue_ - 1) {
        upWaiterWakes();
    }
}


void
Semaphore::up()
{
    if (value_ == maxValue_) {
        {
            Waiter waiter;
            upWaiterList_.appendNode(&waiter);

            auto scopeGuard = MakeScopeGuard([&] () -> void {
                waiter.remove();
            });

            scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
        }

        if (++value_ < maxValue_) {
            upWaiterWakes();
        }
    } else {
        if (++value_ == maxValue_) {
            upWaiterSleeps();
        }
    }

    if (value_ == minValue_ + 1) {
        downWaiterWakes();
    }
}


bool
Semaphore::tryDown() noexcept
{
    if (value_ == minValue_) {
        return false;
    } else {
        if (--value_ == minValue_) {
            downWaiterSleeps();
        }

        if (value_ == maxValue_ - 1) {
            upWaiterWakes();
        }

        return true;
    }
}


bool
Semaphore::tryUp() noexcept
{
    if (value_ == maxValue_) {
        return false;
    } else {
        if (++value_ == maxValue_) {
            upWaiterSleeps();
        }

        if (value_ == minValue_ + 1) {
            downWaiterWakes();
        }

        return true;
    }
}


void
Semaphore::downWaiterWakes() noexcept
{
    if (!downWaiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(downWaiterList_.getTail());
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


void
Semaphore::downWaiterSleeps() noexcept
{
    if (!downWaiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(downWaiterList_.getTail());
        scheduler_->suspendFiber(waiter->fiberHandle);
    }
}


void
Semaphore::upWaiterWakes() noexcept
{
    if (!upWaiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(upWaiterList_.getTail());
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


void
Semaphore::upWaiterSleeps() noexcept
{
    if (!upWaiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(upWaiterList_.getTail());
        scheduler_->suspendFiber(waiter->fiberHandle);
    }
}

} // namespace siren
