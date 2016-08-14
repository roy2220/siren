#pragma once


#include <csetjmp>
#include <cstddef>
#include <exception>
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
    std::function<void ()> procedure;
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

    inline void reset() noexcept;
    inline bool hasAliveFibers() const noexcept;
    inline void *createFiber(const std::function<void ()> &, std::size_t = 0);
    inline void *createFiber(std::function<void ()> &&, std::size_t = 0);
    inline void suspendFiber(void *) noexcept;
    inline void resumeFiber(void *) noexcept;
    inline void *getCurrentFiber() noexcept;
    inline void yieldTo() noexcept;
    inline void yieldToFiber(void *) noexcept;
    inline void run();

private:
    typedef detail::Fiber Fiber;
    typedef detail::FiberState FiberState;

    static constexpr std::size_t MinFiberSize = 4096;

    const std::size_t defaultFiberSize_;
    Fiber idleFiber_;
    List runnableFiberList_;
    Fiber *runningFiber_;
    List suspendedFiberList_;
    std::size_t aliveFiberCount_;
    Fiber *deadFiber_;
    std::exception_ptr exception_;

    inline void finalize() noexcept;
#ifndef NDEBUG
    inline bool isIdle() const noexcept;
#endif

    Fiber *allocateFiber(std::size_t);
    void freeFiber(Fiber *) noexcept;
    void switchToFiber(Fiber *) noexcept;
    [[noreturn]] void runFiber(Fiber *) noexcept;
    [[noreturn]] void fiberStart() noexcept;
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
                                                                     : defaultFiberSize)),
    runningFiber_((idleFiber_.state = FiberState::Running, &idleFiber_)),
    aliveFiberCount_(0),
    deadFiber_(nullptr)
{
}


Scheduler::Scheduler(Scheduler &&other) noexcept
  : defaultFiberSize_(other.defaultFiberSize_),
    runnableFiberList_(std::move(other.runnableFiberList_)),
    runningFiber_((idleFiber_.state = FiberState::Running, &idleFiber_)),
    aliveFiberCount_(0),
    deadFiber_(nullptr)
{
    assert(!other.hasAliveFibers());
}


Scheduler::~Scheduler()
{
    assert(!hasAliveFibers());
    finalize();
}


Scheduler &
Scheduler::operator=(Scheduler &&other) noexcept
{
    if (&other != this) {
        assert(!hasAliveFibers());
        assert(!other.hasAliveFibers());
        finalize();
        runnableFiberList_ = std::move(other.runnableFiberList_);
    }

    return *this;
}


void
Scheduler::finalize() noexcept
{
    SIREN_LIST_FOREACH_SAFE_REVERSE(listNode, runnableFiberList_) {
        auto fiber = static_cast<Fiber *>(listNode);
        freeFiber(fiber);
    }
}


void
Scheduler::reset() noexcept
{
    assert(!hasAliveFibers());
    finalize();
    runnableFiberList_.reset();
}


bool
Scheduler::hasAliveFibers() const noexcept
{
    return aliveFiberCount_ >= 1;
}


#ifndef NDEBUG
bool
Scheduler::isIdle() const noexcept
{
    return runningFiber_ == &idleFiber_;
}
#endif


void *
Scheduler::createFiber(const std::function<void ()> &procedure, std::size_t fiberSize)
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

    fiber->procedure = procedure;
    fiber->context = nullptr;
    runnableFiberList_.addTail((fiber->state = FiberState::Runnable, fiber));
    scopeGuard.dismiss();
    return fiber;
}


void *
Scheduler::createFiber(std::function<void ()> &&procedure, std::size_t fiberSize)
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

    fiber->procedure = procedure;
    fiber->context = nullptr;
    runnableFiberList_.addTail((fiber->state = FiberState::Runnable, fiber));
    scopeGuard.dismiss();
    return fiber;
}


void
Scheduler::suspendFiber(void *fiberHandle) noexcept
{
    assert(fiberHandle != nullptr);
    auto fiber1 = static_cast<Fiber *>(fiberHandle);

    if (fiber1->state == FiberState::Runnable) {
        suspendedFiberList_.addTail((fiber1->state = FiberState::Suspended, fiber1->remove()
                                     , fiber1));
    } else if (fiber1->state == FiberState::Running) {
        suspendedFiberList_.addTail((fiber1->state = FiberState::Suspended, fiber1));
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
        runnableFiberList_.addTail((fiber->state = FiberState::Runnable, fiber->remove(), fiber));
    }
}


void *
Scheduler::getCurrentFiber() noexcept
{
    assert(!isIdle());
    return runningFiber_;
}


void
Scheduler::yieldTo() noexcept
{
    assert(!isIdle());

    if (!idleFiber_.isOnly()) {
        (runningFiber_->state = FiberState::Runnable, runningFiber_)->insertAfter(&idleFiber_);
        auto fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber);
    }
}


void
Scheduler::yieldToFiber(void *fiberHandle) noexcept
{
    assert(!isIdle());
    assert(fiberHandle != nullptr);
    auto fiber = static_cast<Fiber *>(fiberHandle);

    if (fiber->state == FiberState::Runnable) {
        (runningFiber_->state = FiberState::Runnable, runningFiber_)->insertAfter(&idleFiber_);
        switchToFiber(fiber);
    }
}


void
Scheduler::run()
{
    assert(isIdle());

    if (!runnableFiberList_.isEmpty()) {
        runnableFiberList_.addHead((idleFiber_.state = FiberState::Runnable, &idleFiber_));
        auto fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber);

        if (exception_ != nullptr) {
            std::rethrow_exception(std::move(exception_));
        }
    }
}

}
