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
    SIREN_ASSERT(!isWaited());
    initialize();
}


#ifdef SIREN_WITH_DEBUG
bool
Semaphore::isWaited() const noexcept
{
    return !upWaiterList_.isEmpty() || !downWaiterList_.isEmpty();
}
#endif


void
Semaphore::up()
{
    if (value_ < maxValue_) {
        if (++value_ == maxValue_ && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getTail());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }
    } else {
        {
            Waiter waiter;
            upWaiterList_.appendNode(&waiter);

            auto scopeGuard = MakeScopeGuard([&] () -> void {
                waiter.remove();
            });

            scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
        }

        if (++value_ < maxValue_ && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getTail());
            scheduler_->resumeFiber(waiter->fiberHandle);
        }
    }

    if (value_ == minValue_ + 1 && !downWaiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(downWaiterList_.getTail());
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


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

        if (--value_ > minValue_ && !downWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(downWaiterList_.getTail());
            scheduler_->resumeFiber(waiter->fiberHandle);
        }
    } else {
        if (--value_ == minValue_ && !downWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(downWaiterList_.getTail());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }
    }

    if (value_ == maxValue_ - 1 && !upWaiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(upWaiterList_.getTail());
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


bool
Semaphore::tryUp() noexcept
{
    if (value_ < maxValue_) {
        if (++value_ == maxValue_ && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getTail());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }

        if (value_ == minValue_ + 1 && !downWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(downWaiterList_.getTail());
            scheduler_->resumeFiber(waiter->fiberHandle);
        }

        return true;
    } else {
        return false;
    }
}


bool
Semaphore::tryDown() noexcept
{
    if (value_ == minValue_) {
        return false;
    } else {
        if (--value_ == minValue_ && !downWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(downWaiterList_.getTail());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }

        if (value_ == maxValue_ - 1 && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getTail());
            scheduler_->resumeFiber(waiter->fiberHandle);
        }

        return true;
    }
}

} // namespace siren
