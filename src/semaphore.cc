#include "semaphore.h"

#include <cassert>

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
    assert(scheduler != nullptr);
    assert(initialValue >= minValue);
    assert(initialValue <= maxValue);
    initialize();
}


Semaphore::Semaphore(Semaphore &&other) noexcept
  : scheduler_(other.scheduler_),
    initialValue_(other.initialValue_),
    minValue_(other.minValue_),
    maxValue_(other.maxValue_)
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
        assert(initialValue_ == other.initialValue_);
        assert(minValue_ == other.minValue_);
        assert(maxValue_ == other.maxValue_);
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
Semaphore::up()
{
    if (value_ < maxValue_) {
        if (++value_ == maxValue_ && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getHead());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }
    } else {
        {
            Waiter waiter;
            upWaiterList_.appendNode(&waiter);

            auto scopeGuard = MakeScopeGuard([&waiter] () -> void {
                waiter.remove();
            });

            scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
        }

        if (++value_ < maxValue_ && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getHead());
            scheduler_->resumeFiber(waiter->fiberHandle);
        }
    }

    if (value_ == minValue_ + 1 && !downWaiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(downWaiterList_.getHead());
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

            auto scopeGuard = MakeScopeGuard([&waiter] () -> void {
                waiter.remove();
            });

            scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
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
    if (value_ < maxValue_) {
        if (++value_ == maxValue_ && !upWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(upWaiterList_.getHead());
            scheduler_->suspendFiber(waiter->fiberHandle);
        }

        if (value_ == minValue_ + 1 && !downWaiterList_.isEmpty()) {
            auto waiter = static_cast<Waiter *>(downWaiterList_.getHead());
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

} // namespace siren
