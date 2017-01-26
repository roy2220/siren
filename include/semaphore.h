#pragma once


#include <cstdint>
#include <limits>

#include "list.h"


namespace siren {

class Scheduler;
namespace detail { struct SemaphoreWaiter; }


class Semaphore final
{
public:
    explicit Semaphore(Scheduler *, std::intmax_t = 0, std::intmax_t = 0
                              , std::intmax_t = std::numeric_limits<std::intmax_t>::max()) noexcept;
    Semaphore(Semaphore &&) noexcept;
    ~Semaphore();
    Semaphore &operator=(Semaphore &&) noexcept;

    void reset() noexcept;
    void up();
    void down();
    bool tryUp() noexcept;
    bool tryDown() noexcept;

private:
    typedef detail::SemaphoreWaiter Waiter;

    Scheduler *const scheduler_;
    List upWaiterList_;
    List downWaiterList_;
    const std::intmax_t initialValue_;
    const std::intmax_t minValue_;
    const std::intmax_t maxValue_;
    std::intmax_t value_;

    void initialize() noexcept;
    void move(Semaphore *) noexcept;
#ifndef NDEBUG
    bool isWaited() const noexcept;
#endif
};

} // namespace siren
