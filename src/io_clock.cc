#include "io_clock.h"


namespace siren {

void
IOClock::getExpiredTimers(std::vector<Timer *> *timers)
{
    assert(timers != nullptr);

    for (HeapNode *heapNode = timerHeap_.getTop(); heapNode != nullptr
         ; heapNode = timerHeap_.getTop()) {
        auto timer = static_cast<Timer *>(heapNode);

        if (timer->expiryTime_ > now_) {
            return;
        } else {
            timerHeap_.removeTop();
            timers->push_back(timer);
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

}
