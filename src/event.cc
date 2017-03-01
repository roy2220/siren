#include "event.h"

#include "assert.h"
#include "config.h"
#include "scheduler.h"
#include "scope_guard.h"


namespace siren {

namespace detail {

struct EventWaiter
  : ListNode
{
    void *fiberHandle;
};

} // namespace detail


Event::Event(Scheduler *scheduler) noexcept
  : scheduler_(scheduler)
{
    SIREN_ASSERT(scheduler != nullptr);
    initialize();
}


Event::Event(Event &&other) noexcept
  : scheduler_(other.scheduler_)
{
    SIREN_ASSERT(!other.isWaited());
    other.move(this);
}


Event::~Event()
{
    SIREN_ASSERT(!isWaited());
}


Event &
Event::operator=(Event &&other) noexcept
{
    if (&other != this) {
        SIREN_ASSERT(!isWaited());
        SIREN_ASSERT(!other.isWaited());
        other.move(this);
    }

    return *this;
}


void
Event::initialize() noexcept
{
    hasOccurred_ = false;
}


void
Event::move(Event *other) noexcept
{
    other->hasOccurred_ = hasOccurred_;
    initialize();
}


void
Event::reset() noexcept
{
    if (hasOccurred_) {
        hasOccurred_ = false;
        waiterSleeps();
    }
}


#ifdef SIREN_WITH_DEBUG
bool
Event::isWaited() const noexcept
{
    return !waiterList_.isEmpty();
}
#endif


void
Event::trigger() noexcept
{
    if (!hasOccurred_) {
        hasOccurred_ = true;
        waiterWakes();
    }
}


void
Event::waitFor()
{
    if (!hasOccurred_) {
        {
            Waiter waiter;
            waiterList_.appendNode(&waiter);

            auto scopeGuard = MakeScopeGuard([&] () -> void {
                waiter.remove();
            });

            scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
        }

        waiterWakes();
    }
}


bool
Event::tryWaitFor() noexcept
{
    return hasOccurred_;
}


void
Event::waiterWakes() noexcept
{
    if (!waiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(waiterList_.getTail());
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


void
Event::waiterSleeps() noexcept
{
    if (!waiterList_.isEmpty()) {
        auto waiter = static_cast<Waiter *>(waiterList_.getTail());
        scheduler_->suspendFiber(waiter->fiberHandle);
    }
}

} // namespace siren
