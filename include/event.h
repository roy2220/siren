#pragma once


#include "config.h"
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

    void reset() noexcept;
    void trigger() noexcept;
    void waitFor();
    bool tryWaitFor() noexcept;

private:
    typedef detail::EventWaiter Waiter;

    Scheduler *const scheduler_;
    List waiterList_;
    bool hasOccurred_;

    void initialize() noexcept;
    void move(Event *) noexcept;
#ifdef SIREN_WITH_DEBUG
    bool isWaited() const noexcept;
#endif
    void waiterWakes() noexcept;
    void waiterSleeps() noexcept;
};

} // namespace siren
