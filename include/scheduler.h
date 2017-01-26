#pragma once


#include <csetjmp>
#include <cstddef>
#include <chrono>
#include <exception>
#include <functional>

#include "list.h"


namespace siren {

namespace detail { enum class FiberState; }


namespace detail {

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
    bool isBackground;
    bool isPreInterrupted;
    bool isPostInterrupted;
    std::chrono::steady_clock::time_point creationTime;
};

} // namespace detail


class Scheduler final
{
public:
    inline std::size_t getNumberOfAliveFibers() const noexcept;
    inline std::size_t getNumberOfForegroundFibers() const noexcept;
    inline std::size_t getNumberOfBackgroundFibers() const noexcept;
    inline std::size_t getNumberOfActiveFibers() const noexcept;
    inline void *getCurrentFiber() noexcept;

    template <class T>
    inline void *createFiber(T &&, std::size_t = 0, bool = false);

    explicit Scheduler(std::size_t = 0) noexcept;
    Scheduler(Scheduler &&) noexcept;
    ~Scheduler();
    Scheduler &operator=(Scheduler &&) noexcept;

    void reset() noexcept;
    void suspendFiber(void *);
    void resumeFiber(void *) noexcept;
    void interruptFiber(void *);
    void yieldToFiber(void *);
    void yieldTo();
    void run();

private:
    typedef detail::Fiber Fiber;
    typedef detail::FiberState FiberState;

    static constexpr std::size_t MinFiberSize = 4096;

    const std::size_t defaultFiberSize_;
    Fiber idleFiber_;
    Fiber *currentFiber_;
    Fiber *deadFiber_;
    List runnableFiberList_;
    List suspendedFiberList_;
    std::size_t aliveFiberCount_;
    std::size_t backgroundFiberCount_;
    std::size_t activeFiberCount_;
    std::exception_ptr exception_;

    void initialize() noexcept;
    void finalize();
    void move(Scheduler *) noexcept;
#ifndef NDEBUG
    bool isIdle() const noexcept;
#endif
    void destroyFiber(Fiber *) noexcept;
    Fiber *allocateFiber(std::size_t);
    void freeFiber(Fiber *) noexcept;
    void switchToFiber(Fiber *);
    [[noreturn]] void runFiber(Fiber *) noexcept;
    void onFiberPreRun();
    void onFiberPostRun();
    [[noreturn]] void fiberStart() noexcept;
};


struct FiberInterruption
{
};

} // namespace siren


/*
 * #include "scheduler-inl.h"
 */


#include <cassert>
#include <algorithm>
#include <utility>

#include "helper_macros.h"
#include "scope_guard.h"


namespace siren {

namespace detail {

enum class FiberState
{
    Runnable,
    Running,
    Suspended,
};

} // namespace detail


std::size_t
Scheduler::getNumberOfAliveFibers() const noexcept
{
    return aliveFiberCount_;
}


std::size_t
Scheduler::getNumberOfForegroundFibers() const noexcept
{
    return aliveFiberCount_ - backgroundFiberCount_;
}


std::size_t
Scheduler::getNumberOfBackgroundFibers() const noexcept
{
    return backgroundFiberCount_;
}


std::size_t
Scheduler::getNumberOfActiveFibers() const noexcept
{
    return activeFiberCount_;
}


template <class T>
void *
Scheduler::createFiber(T &&procedure, std::size_t fiberSize, bool fiberIsBackground)
{
    fiberSize = SIREN_ALIGN(fiberSize, alignof(std::max_align_t));

    if (fiberSize == 0) {
        fiberSize = defaultFiberSize_;
    } else {
        fiberSize = std::max(fiberSize, std::size_t(MinFiberSize));
    }

    Fiber *fiber = allocateFiber(fiberSize);

    auto scopeGuard = MakeScopeGuard([this, fiber] () -> void {
        freeFiber(fiber);
    });

    fiber->procedure = std::forward<T>(procedure);
    fiber->context = nullptr;
    runnableFiberList_.appendNode((fiber->state = FiberState::Runnable, fiber));
    fiber->isBackground = fiberIsBackground;
    fiber->isPreInterrupted = fiber->isPostInterrupted = false;
    fiber->creationTime = std::chrono::steady_clock::now();
    ++aliveFiberCount_;

    if (fiberIsBackground) {
        ++backgroundFiberCount_;
    }

    scopeGuard.dismiss();
    return fiber;
}


void *
Scheduler::getCurrentFiber() noexcept
{
    assert(!isIdle());
    return currentFiber_;
}

} // namespace siren
