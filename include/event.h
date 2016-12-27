#pragma once


#include "list.h"


namespace siren {

class Scheduler;


namespace detail {

struct EventWaiter
  : ListNode
{
    void *fiberHandle;
};

}


class Event final
{
public:
    inline explicit Event(Scheduler *) noexcept;
    inline ~Event();

    inline void trigger() noexcept;
    inline void waitFor();

private:
    typedef detail::EventWaiter Waiter;

    Scheduler *scheduler_;
    List waiterList_;

    Event(const Event &) = delete;
    Event &operator=(const Event &) = delete;

#ifndef NDEBUG
    inline bool isWaited() const noexcept;
#endif
};

}


/*
 * #include "event-inl.h"
 */


#include <cassert>

#include "scheduler.h"
#include "scope_guard.h"


namespace siren {

Event::Event(Scheduler *scheduler) noexcept
  : scheduler_(scheduler)
{
    assert(scheduler != nullptr);
}


Event::~Event()
{
    assert(!isWaited());
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
    SIREN_LIST_FOREACH(listNode, waiterList_) {
        auto waiter = static_cast<Waiter *>(listNode);
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


void
Event::waitFor()
{
    Waiter waiter;
    waiterList_.addTail(&waiter);

    auto scopeGuard = MakeScopeGuard([&waiter] () -> void {
        waiter.remove();
    });

    scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
}

}
