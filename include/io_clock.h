#pragma once


#include <chrono>
#include <vector>

#include "heap.h"


namespace siren {

class IOTimer;


class IOClock final
{
public:
    typedef IOTimer Timer;

    inline std::chrono::milliseconds getDueTime() const noexcept;

    explicit IOClock() noexcept;
    IOClock(IOClock &&) noexcept;
    IOClock &operator=(IOClock &&) noexcept;

    void reset() noexcept;
    void start() noexcept;
    void stop() noexcept;
    void restart() noexcept;
    void addTimer(Timer *, std::chrono::milliseconds);
    void removeTimer(Timer *) noexcept;
    void getExpiredTimers(std::vector<Timer *> *);

private:
    Heap timerHeap_;
    std::chrono::milliseconds now_;
    std::chrono::steady_clock::time_point startTime_;

    void initialize() noexcept;
    void move(IOClock *) noexcept;
};


class IOTimer
  : private HeapNode
{
protected:
    inline explicit IOTimer() noexcept;

    ~IOTimer() = default;

private:
    std::chrono::milliseconds expiryTime_;

    static bool OrderHeapNode(const HeapNode *, const HeapNode *) noexcept;

    IOTimer(const IOTimer &) = delete;
    IOTimer &operator=(const IOTimer &) = delete;

    friend IOClock;
};

} // namespace siren


/*
 * #include "io_clock-inl.h"
 */


#include <algorithm>


namespace siren {

std::chrono::milliseconds
IOClock::getDueTime() const noexcept
{
    if (timerHeap_.isEmpty()) {
        return std::chrono::milliseconds(-1);
    } else {
        auto timer = static_cast<const IOTimer *>(timerHeap_.getTop());
        return std::max(timer->expiryTime_ - now_, std::chrono::milliseconds(0));
    }
}


IOTimer::IOTimer() noexcept
{
}

} // namespace siren
