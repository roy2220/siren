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
    inline Event(Event &&) noexcept;
    inline ~Event();
    inline Event &operator=(Event &&) noexcept;

    inline void trigger() noexcept;
    inline void waitFor();

private:
    typedef detail::EventWaiter Waiter;

    Scheduler *const scheduler_;
    List waiterList_;

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


Event::Event(Event &&other) noexcept
  : scheduler_(other.scheduler_)
{
    assert(!other.isWaited());
}


Event::~Event()
{
    assert(!isWaited());
}


Event &Event::operator=(Event &&other) noexcept
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
    SIREN_LIST_FOREACH(listNode, waiterList_) {
        auto waiter = static_cast<Waiter *>(listNode);
        scheduler_->resumeFiber(waiter->fiberHandle);
    }
}


void
Event::waitFor()
{
    Waiter waiter;
    waiterList_.appendNode(&waiter);

    auto scopeGuard = MakeScopeGuard([&waiter] () -> void {
        waiter.remove();
    });

    scheduler_->suspendFiber(waiter.fiberHandle = scheduler_->getCurrentFiber());
}

}
