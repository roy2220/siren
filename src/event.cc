#include "event.h"

#include <cassert>

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
    assert(scheduler != nullptr);
}


Event::Event(Event &&other) noexcept
  : scheduler_(other.scheduler_)
{
    assert(!other.isWaited());
}


Event::~Event()
{
    assert(!isWaited());
}


Event &
Event::operator=(Event &&other) noexcept
{
    if (&other != this) {
        assert(!isWaited());
        assert(!other.isWaited());
    }

    return *this;
}


#ifndef NDEBUG
bool
Event::isWaited() const noexcept
{
    return !waiterList_.isEmpty();
}
#endif


void
Event::trigger() noexcept
{
    SIREN_LIST_FOREACH_REVERSE(listNode, waiterList_) {
        auto waiter = static_cast<Waiter *>(listNode);
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


void
Event::waitFor()
{
    Waiter waiter;
    waiterList_.appendNode(&waiter);

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        waiter.remove();
    });

    scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
}

} // namespace siren
