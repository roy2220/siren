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

    inline explicit IOClock() noexcept;
    inline IOClock(IOClock &&) noexcept;
    inline IOClock &operator=(IOClock &&) noexcept;

    inline void reset() noexcept;
    inline void start() noexcept;
    inline void stop() noexcept;
    inline void addTimer(Timer *, std::chrono::milliseconds);
    inline void removeTimer(Timer *) noexcept;

    void getExpiredTimers(std::vector<Timer *> *);

private:
    Heap timerHeap_;
    std::chrono::milliseconds time_;
    std::chrono::steady_clock::time_point startTime_;

    inline void initialize() noexcept;
    inline void move(IOClock *) noexcept;
};


class IOTimer
  : private HeapNode
{
protected:
    inline explicit IOTimer() noexcept;

    ~IOTimer() = default;

private:
    std::chrono::milliseconds expiryTime_;

    static bool OrderHeapNode(const HeapNode *, const HeapNode *);

    IOTimer(const IOTimer &) = delete;
    IOTimer &operator=(const IOTimer &) = delete;

    friend IOClock;
};

}


/*
 * #include "io_clock-inl.h"
 */


#include <cassert>
#include <utility>


namespace siren {

IOClock::IOClock() noexcept
  : timerHeap_(Timer::OrderHeapNode)
{
    initialize();
}


IOClock::IOClock(IOClock &&other) noexcept
  : timerHeap_(std::move(other.timerHeap_))
{
    other.move(this);
}


IOClock &
IOClock::operator=(IOClock &&other) noexcept
{
    if (&other != this) {
        timerHeap_ = std::move(other.timerHeap_);
        other.move(this);
    }

    return *this;
}


void
IOClock::initialize() noexcept
{
    time_ = std::chrono::milliseconds(0);
    startTime_ = std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration(-1));
}


void
IOClock::move(IOClock *other) noexcept
{
    other->time_ = time_;
    other->startTime_ = startTime_;
    initialize();
}


void
IOClock::reset() noexcept
{
    timerHeap_.reset();
    initialize();
}


void
IOClock::start() noexcept
{
    assert(startTime_.time_since_epoch().count() < 0);
    startTime_ = std::chrono::steady_clock::now();
}


void
IOClock::stop() noexcept
{
    assert(startTime_.time_since_epoch().count() >= 0);
    std::chrono::steady_clock::time_point stopTime = std::chrono::steady_clock::now();
    time_ += std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime_);
    startTime_ = std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration(-1));
}


void
IOClock::addTimer(Timer *timer, std::chrono::milliseconds interval)
{
    assert(timer != nullptr);
    timer->expiryTime_ = time_ + interval;
    timerHeap_.addNode(timer);
}


void
IOClock::removeTimer(Timer *timer) noexcept
{
    assert(timer != nullptr);
    timerHeap_.removeNode(timer);
}


IOTimer::IOTimer() noexcept
{
}

}
