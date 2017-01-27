#include "io_clock.h"

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
    now_ = std::chrono::milliseconds(0);
#ifndef NDEBUG
    startTime_ = std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration(-1));
#endif
}


void
IOClock::move(IOClock *other) noexcept
{
    other->now_ = now_;
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
    now_ += std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime_);
#ifndef NDEBUG
    startTime_ = std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration(-1));
#endif
}


void
IOClock::restart() noexcept
{
    assert(startTime_.time_since_epoch().count() >= 0);
    std::chrono::steady_clock::time_point stopTime = std::chrono::steady_clock::now();
    now_ += std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime_);
    startTime_ = stopTime;
}


void
IOClock::addTimer(Timer *timer, std::chrono::milliseconds interval)
{
    assert(timer != nullptr);

    if (interval.count() < 0) {
        timer->expiryTime_ = std::chrono::milliseconds::max();
    } else {
        timer->expiryTime_ = now_ + interval;
    }

    timerHeap_.insertNode(timer);
}


void
IOClock::removeTimer(Timer *timer) noexcept
{
    assert(timer != nullptr);
    timerHeap_.removeNode(timer);
}


void
IOClock::getExpiredTimers(std::vector<Timer *> *timers)
{
    assert(timers != nullptr);

    for (HeapNode *heapNode = timerHeap_.getTop(); heapNode != nullptr
         ; heapNode = timerHeap_.getTop()) {
        auto timer = static_cast<Timer *>(heapNode);

        if (timer->expiryTime_ <= now_) {
            timerHeap_.removeTop();
            timers->push_back(timer);
        } else {
            return;
        }
    }
}


bool
IOTimer::OrderHeapNode(const HeapNode *HeapNode1, const HeapNode *HeapNode2) noexcept
{
    auto timer1 = static_cast<const IOTimer *>(HeapNode1);
    auto timer2 = static_cast<const IOTimer *>(HeapNode2);
    return timer1->expiryTime_ <= timer2->expiryTime_;
}

} // namespace siren
