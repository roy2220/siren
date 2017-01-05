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
    bool isBackground;
    bool isPreInterrupted;
    bool isPostInterrupted;
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
    inline std::size_t getNumberOfAliveFibers() const noexcept;
    inline std::size_t getNumberOfForegroundFibers() const noexcept;
    inline std::size_t getNumberOfBackgroundFibers() const noexcept;
    inline std::size_t getNumberOfActiveFibers() const noexcept;
    inline void suspendFiber(void *);
    inline void resumeFiber(void *) noexcept;
    inline void interruptFiber(void *);
    inline void *getCurrentFiber() noexcept;
    inline void yieldTo();
    inline void yieldToFiber(void *);
    inline void run();

    template <class T>
    inline void *createFiber(T &&, std::size_t = 0, bool = false);

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

    inline void initialize() noexcept;
    inline void finalize();
    inline void move(Scheduler *) noexcept;
#ifndef NDEBUG
    inline bool isIdle() const noexcept;
#endif
    inline void destroyFiber(Fiber *) noexcept;

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

}


/*
 * #include "scheduler-inl.h"
 */


#include <cassert>
#include <algorithm>
#include <utility>

#include "helper_macros.h"
#include "scope_guard.h"


namespace siren {

Scheduler::Scheduler(std::size_t defaultFiberSize) noexcept
  : defaultFiberSize_(std::max(SIREN_ALIGN(defaultFiberSize, alignof(std::max_align_t))
                               , std::size_t(MinFiberSize))),
    currentFiber_((idleFiber_.state = FiberState::Running, &idleFiber_)),
    deadFiber_(nullptr),
    activeFiberCount_(0)
{
    idleFiber_.isPreInterrupted = idleFiber_.isPostInterrupted = false;
    initialize();
}


Scheduler::Scheduler(Scheduler &&other) noexcept
  : defaultFiberSize_(other.defaultFiberSize_),
    currentFiber_((idleFiber_.state = FiberState::Running, &idleFiber_)),
    deadFiber_(nullptr),
    runnableFiberList_(std::move(other.runnableFiberList_)),
    suspendedFiberList_(std::move(other.suspendedFiberList_)),
    activeFiberCount_(0)
{
    assert(other.activeFiberCount_ == 0);
    idleFiber_.isPreInterrupted = idleFiber_.isPostInterrupted = false;
    other.move(this);
}


Scheduler::~Scheduler()
{
    assert(isIdle());
    finalize();
}


Scheduler &
Scheduler::operator=(Scheduler &&other) noexcept
{
    if (&other != this) {
        assert(isIdle());
        assert(other.activeFiberCount_ == 0);
        finalize();
        runnableFiberList_ = std::move(other.runnableFiberList_);
        suspendedFiberList_ = std::move(other.suspendedFiberList_);
        other.move(this);
    }

    return *this;
}


void
Scheduler::initialize() noexcept
{
    aliveFiberCount_ = 0;
    backgroundFiberCount_ = 0;
}


void
Scheduler::finalize()
{
    {
        List list = std::move(runnableFiberList_);

        SIREN_LIST_FOREACH_SAFE_REVERSE(listNode, list) {
            auto fiber = static_cast<Fiber *>(listNode);
            interruptFiber(fiber);
        }
    }

    {
        List list = std::move(suspendedFiberList_);

        SIREN_LIST_FOREACH_SAFE_REVERSE(listNode, list) {
            auto fiber = static_cast<Fiber *>(listNode);
            interruptFiber(fiber);
        }
    }

    if (aliveFiberCount_ >= 1) {
        std::terminate();
    }
}


void
Scheduler::move(Scheduler *other) noexcept
{
    other->aliveFiberCount_ = aliveFiberCount_;
    other->backgroundFiberCount_ = backgroundFiberCount_;
    initialize();
}


void
Scheduler::reset() noexcept
{
    assert(isIdle());
    finalize();
}


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


#ifndef NDEBUG
bool
Scheduler::isIdle() const noexcept
{
    return currentFiber_ == &idleFiber_;
}
#endif


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
    runnableFiberList_.addTail((fiber->state = FiberState::Runnable, fiber));
    fiber->isBackground = fiberIsBackground;
    fiber->isPreInterrupted = fiber->isPostInterrupted = false;
    ++aliveFiberCount_;

    if (fiberIsBackground) {
        ++backgroundFiberCount_;
    }

    scopeGuard.dismiss();
    return fiber;
}


void
Scheduler::destroyFiber(Fiber *fiber) noexcept
{
    bool fiberIsBackground = fiber->isBackground;
    freeFiber(fiber);
    --aliveFiberCount_;

    if (fiberIsBackground) {
        --backgroundFiberCount_;
    }
}


void
Scheduler::suspendFiber(void *fiberHandle)
{
    assert(fiberHandle != nullptr);
    auto fiber = static_cast<Fiber *>(fiberHandle);

    if (fiber->state == FiberState::Runnable) {
        suspendedFiberList_.addTail((fiber->state = FiberState::Suspended, fiber->remove(), fiber));
    } else if (fiber->state == FiberState::Running) {
        suspendedFiberList_.addTail((currentFiber_->state = FiberState::Suspended, currentFiber_));

        auto scopeGuard = MakeScopeGuard([this] () -> void {
            (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
        });

        fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber);
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


void
Scheduler::interruptFiber(void *fiberHandle)
{
    assert(fiberHandle != nullptr);
    auto fiber = static_cast<Fiber *>(fiberHandle);

    if (fiber->state == FiberState::Running) {
        fiber->isPostInterrupted = true;
    } else {
        if (fiber->state == FiberState::Runnable) {
            fiber->isPostInterrupted = true;
        } else {
            fiber->isPreInterrupted = true;
        }

        runnableFiberList_.addTail((currentFiber_->state = FiberState::Runnable, currentFiber_));

        auto scopeGuard = MakeScopeGuard([this] () -> void {
            (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
        });

        switchToFiber(fiber);

        if (exception_ != nullptr) {
            std::rethrow_exception(std::move(exception_));
        }
    }
}


void *
Scheduler::getCurrentFiber() noexcept
{
    assert(!isIdle());
    return currentFiber_;
}


void
Scheduler::yieldTo()
{
    assert(!isIdle());

    if (!idleFiber_.isOnly()) {
        (currentFiber_->state = FiberState::Runnable, currentFiber_)->insertAfter(&idleFiber_);

        auto scopeGuard = MakeScopeGuard([this] () -> void {
            (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
        });

        auto fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber);
    }
}


void
Scheduler::yieldToFiber(void *fiberHandle)
{
    assert(!isIdle());
    assert(fiberHandle != nullptr);
    auto fiber = static_cast<Fiber *>(fiberHandle);

    if (fiber->state == FiberState::Runnable) {
        (currentFiber_->state = FiberState::Runnable, currentFiber_)->insertAfter(&idleFiber_);

        auto scopeGuard = MakeScopeGuard([this] () -> void {
            (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
        });

        switchToFiber(fiber);
    }
}


void
Scheduler::run()
{
    assert(isIdle());

    if (!runnableFiberList_.isEmpty()) {
        runnableFiberList_.addHead((currentFiber_->state = FiberState::Runnable, currentFiber_));

        auto scopeGuard = MakeScopeGuard([this] () -> void {
            (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
        });

        auto fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber);

        if (exception_ != nullptr) {
            std::rethrow_exception(std::move(exception_));
        }
    }
}

}
