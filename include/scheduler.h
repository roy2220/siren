#pragma once


#include <csetjmp>
#include <cstddef>
#include <functional>

#include "list.h"


namespace siren {

namespace detail {

enum class FiberState
{
    Runnable,
    Running,
    Suspended
};


struct alignas(std::max_align_t) Fiber
  : ListNode
{
    typedef FiberState State;

    char *stack;
    std::size_t stackSize;
#ifdef USE_VALGRIND
    int stackID;
#endif
    std::function<void ()> routine;
    std::jmp_buf *context;
    State state;
};

}


class Scheduler final
{
public:
    inline explicit Scheduler(std::size_t = 0) noexcept;
    inline Scheduler(Scheduler &&) noexcept;
    inline ~Scheduler();
    inline Scheduler &operator=(Scheduler &&) noexcept;

    inline bool allFibersHaveExited() const noexcept;
    inline void *createFiber(const std::function<void ()> &, std::size_t = 0);
    inline void *createFiber(std::function<void ()> &&, std::size_t = 0);
    inline void *getCurrentFiber() noexcept;
    inline void suspendFiber(void *) noexcept;
    inline void resumeFiber(void *) noexcept;
    inline void currentFiberYields() noexcept;
    [[noreturn]] inline void currentFiberExits() noexcept;
    inline void run() noexcept;

private:
    typedef detail::Fiber Fiber;
    typedef detail::FiberState FiberState;

    static constexpr std::size_t MinFiberSize = 4096;

    static void FiberStart(Fiber *) noexcept;

    const std::size_t defaultFiberSize_;
    Fiber idleFiber_;
    List runnableFiberList_;
    Fiber *runningFiber_;
    Fiber *fiberToFree_;
    std::size_t numberOfFibers_;

    inline void initialize() noexcept;
    inline void move(Scheduler *) noexcept;
#ifndef NDEBUG
    inline bool isIdle() const noexcept;
#endif

    Fiber *allocateFiber(std::size_t);
    void freeFiber(Fiber *) noexcept;
    void switchToFiber(Fiber *) noexcept;
    [[noreturn]] void runFiber(Fiber *) noexcept;
};

}


/*
 * #include "scheduler-inl.h"
 */


#include <cassert>
#include <utility>

#include "next_power_of_two.h"
#include "scope_guard.h"


namespace siren {

Scheduler::Scheduler(std::size_t defaultFiberSize) noexcept
  : defaultFiberSize_(NextPowerOfTwo(defaultFiberSize < MinFiberSize ? MinFiberSize
                                                                     : defaultFiberSize))
{
    idleFiber_.state = FiberState::Running;
    runningFiber_ = &idleFiber_;
    fiberToFree_ = nullptr;
    initialize();
}


Scheduler::Scheduler(Scheduler &&other) noexcept
  : defaultFiberSize_(other.defaultFiberSize_),
    runnableFiberList_(std::move(other.runnableFiberList_))
{
    assert(other.isIdle());
    idleFiber_.state = FiberState::Running;
    runningFiber_ = &idleFiber_;
    fiberToFree_ = nullptr;
    other.move(this);
}


Scheduler::~Scheduler()
{
    assert(allFibersHaveExited());
}


Scheduler &
Scheduler::operator=(Scheduler &&other) noexcept
{
    if (&other != this) {
        assert(allFibersHaveExited());
        assert(other.isIdle());
        runnableFiberList_ = std::move(other.runnableFiberList_);
        other.move(this);
    }

    return *this;
}


void
Scheduler::initialize() noexcept
{
    numberOfFibers_ = 0;
}


void
Scheduler::move(Scheduler *other) noexcept
{
    other->numberOfFibers_ = numberOfFibers_;
    initialize();
}


#ifndef NDEBUG
bool
Scheduler::isIdle() const noexcept
{
    return runningFiber_ == &idleFiber_;
}
#endif


bool
Scheduler::allFibersHaveExited() const noexcept
{
    return numberOfFibers_ == 0;
}


void *
Scheduler::createFiber(const std::function<void ()> &routine, std::size_t fiberSize)
{
    if (fiberSize == 0) {
        fiberSize = defaultFiberSize_;
    } else {
        fiberSize = NextPowerOfTwo(fiberSize < MinFiberSize ? MinFiberSize : fiberSize);
    }

    Fiber *fiber = allocateFiber(fiberSize);

    auto scopeGuard = MakeScopeGuard([this, fiber] () -> void {
        freeFiber(fiber);
    });

    fiber->routine = routine;
    fiber->context = nullptr;
    fiber->state = FiberState::Runnable;
    scopeGuard.dismiss();
    ++numberOfFibers_;
    runnableFiberList_.addTail(fiber);
    return fiber;
}


void *
Scheduler::createFiber(std::function<void ()> &&routine, std::size_t fiberSize)
{
    if (fiberSize == 0) {
        fiberSize = defaultFiberSize_;
    } else {
        fiberSize = NextPowerOfTwo(fiberSize < MinFiberSize ? MinFiberSize : fiberSize);
    }

    Fiber *fiber = allocateFiber(fiberSize);

    auto scopeGuard = MakeScopeGuard([this, fiber] () -> void {
        freeFiber(fiber);
    });

    fiber->routine = std::move(routine);
    fiber->context = nullptr;
    fiber->state = FiberState::Runnable;
    scopeGuard.dismiss();
    ++numberOfFibers_;
    runnableFiberList_.addTail(fiber);
    return fiber;
}


void *
Scheduler::getCurrentFiber() noexcept
{
    assert(!isIdle());
    return runningFiber_;
}


void
Scheduler::suspendFiber(void *fiberHandle) noexcept
{
    assert(fiberHandle != nullptr);
    auto fiber1 = static_cast<Fiber *>(fiberHandle);

    if (fiber1->state == FiberState::Runnable) {
        fiber1->state = FiberState::Suspended;
        fiber1->remove();
    } else if (fiber1->state == FiberState::Running) {
        fiber1->state = FiberState::Suspended;
        auto fiber2 = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber2);
    }
}


void
Scheduler::resumeFiber(void *fiberHandle) noexcept
{
    assert(fiberHandle != nullptr);
    auto fiber = static_cast<Fiber *>(fiberHandle);

    if (fiber->state == FiberState::Suspended) {
        fiber->state = FiberState::Runnable;
        runnableFiberList_.addTail(fiber);
    }
}


void
Scheduler::currentFiberYields() noexcept
{
    assert(!isIdle());

    if (!idleFiber_.isOnly()) {
        runningFiber_->state = FiberState::Runnable;
        runningFiber_->insertAfter(&idleFiber_);
        auto fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber);
    }
}


void
Scheduler::currentFiberExits() noexcept
{
    assert(!isIdle());
    fiberToFree_ = runningFiber_;
    --numberOfFibers_;
    auto fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
    runFiber(fiber);
}


void
Scheduler::run() noexcept
{
    assert(isIdle());

    if (!runnableFiberList_.isEmpty()) {
        idleFiber_.state = FiberState::Runnable;
        runnableFiberList_.addHead(&idleFiber_);
        auto fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber);
    }
}

}
