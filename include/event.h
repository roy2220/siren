#pragma once


#include "list.h"


namespace siren {

class Scheduler;
namespace detail { struct EventWaiter; }


class Event final
{
public:
    explicit Event(Scheduler *) noexcept;
    Event(Event &&) noexcept;
    ~Event();
    Event &operator=(Event &&) noexcept;

    void trigger() noexcept;
    void waitFor();

private:
    typedef detail::EventWaiter Waiter;

    Scheduler *const scheduler_;
    List waiterList_;

#ifndef NDEBUG
    bool isWaited() const noexcept;
#endif
};

} // namespace siren
