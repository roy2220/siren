#include "scheduler.h"

#include <cerrno>
#include <cstdlib>
#include <system_error>

#ifdef USE_VALGRIND
#    include <valgrind/valgrind.h>
#endif


namespace siren {

void
Scheduler::FiberStartWrapper(Scheduler *self) noexcept
{
    self->fiberStart();
}


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
        suspendedFiberList_.append(&list);

        list.sort([] (const ListNode *listNode1, const ListNode *listNode2) -> bool {
            auto fiber1 = static_cast<const Fiber *>(listNode1);
            auto fiber2 = static_cast<const Fiber *>(listNode2);
            return fiber1->creationTime <= fiber2->creationTime;
        });

        while (!list.isEmpty()) {
            auto fiber = static_cast<Fiber *>(list.getTail());
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


#ifndef NDEBUG
bool
Scheduler::isIdle() const noexcept
{
    return currentFiber_ == &idleFiber_;
}
#endif


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


Scheduler::Fiber *
Scheduler::allocateFiber(std::size_t fiberSize)
{
    auto base = static_cast<char *>(std::malloc(fiberSize));

    if (base == nullptr) {
        throw std::system_error(errno, std::system_category(), "malloc() failed");
    }

    std::size_t fiberOffset, stackOffset, stackSize;
#if defined(__i386__) || defined(__x86_64__)
    fiberOffset = fiberSize - sizeof(Fiber);
    stackOffset = 0;
    stackSize = fiberOffset;
#else
#    error architecture not supported
#endif
    auto fiber = new (base + fiberOffset) Fiber();
    fiber->stack = base + stackOffset;
    fiber->stackSize = stackSize;
#ifdef USE_VALGRIND
    fiber->stackID = VALGRIND_STACK_REGISTER(fiber->stack, fiber->stack + fiber->stackSize);
#endif
    return fiber;
}


void
Scheduler::freeFiber(Fiber *fiber) noexcept
{
    char *base;
#if defined(__i386__) || defined(__x86_64__)
    base = fiber->stack;
#else
#    error architecture not supported
#endif
#ifdef USE_VALGRIND
    VALGRIND_STACK_DEREGISTER(fiber->stackID);
#endif
    fiber->~Fiber();
    std::free(base);
}


void
Scheduler::suspendFiber(void *fiberHandle)
{
    assert(fiberHandle != nullptr);
    auto fiber = static_cast<Fiber *>(fiberHandle);

    if (fiber->state == FiberState::Runnable) {
        suspendedFiberList_.appendNode((fiber->state = FiberState::Suspended, fiber->remove()
                                        , fiber));
    } else if (fiber->state == FiberState::Running) {
        suspendedFiberList_.appendNode((currentFiber_->state = FiberState::Suspended
                                        , currentFiber_));

        auto scopeGuard = MakeScopeGuard([&] () -> void {
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
        runnableFiberList_.appendNode((fiber->state = FiberState::Runnable, fiber->remove()
                                       , fiber));
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

        runnableFiberList_.appendNode((currentFiber_->state = FiberState::Runnable, currentFiber_));

        auto scopeGuard = MakeScopeGuard([&] () -> void {
            (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
        });

        switchToFiber(fiber);

        if (exception_ != nullptr) {
            std::rethrow_exception(std::move(exception_));
        }
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

        auto scopeGuard = MakeScopeGuard([&] () -> void {
            (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
        });

        switchToFiber(fiber);
    }
}


void
Scheduler::yieldTo()
{
    assert(!isIdle());

    if (!idleFiber_.isOnly()) {
        (currentFiber_->state = FiberState::Runnable, currentFiber_)->insertAfter(&idleFiber_);

        auto scopeGuard = MakeScopeGuard([&] () -> void {
            (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
        });

        auto fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber);
    }
}


void
Scheduler::run()
{
    assert(isIdle());

    if (!runnableFiberList_.isEmpty()) {
        runnableFiberList_.prependNode((currentFiber_->state = FiberState::Runnable
                                        , currentFiber_));

        auto scopeGuard = MakeScopeGuard([&] () -> void {
            (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
        });

        auto fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
        switchToFiber(fiber);

        if (exception_ != nullptr) {
            std::rethrow_exception(std::move(exception_));
        }
    }
}


void
Scheduler::switchToFiber(Fiber *fiber)
{
    onFiberPostRun();

    {
        std::jmp_buf context;

        if (setjmp(context) == 0) {
            currentFiber_->context = &context;
            runFiber(fiber);
        }
    }

    onFiberPreRun();
}


void
Scheduler::runFiber(Fiber *fiber) noexcept
{
    currentFiber_ = fiber;

    if (fiber->context == nullptr) {
#if defined(__GNUG__)
        __asm__ __volatile__ (
#    if defined(__i386__)
            "movl\t$0, %%ebp\n\t"
            "movl\t%0, %%esp\n\t"
            "pushl\t%1\n\t"
            "pushl\t$0\n\t"
            "jmpl\t*%2"
            :
            : "r"(fiber->stack + fiber->stackSize), "r"(this), "r"(FiberStartWrapper)
#    elif defined(__x86_64__)
            "movq\t$0, %%rbp\n\t"
            "movq\t%0, %%rsp\n\t"
            "pushq\t$0\n\t"
            "jmpq\t*%2"
            :
            : "r"(fiber->stack + fiber->stackSize), "D"(this), "r"(FiberStartWrapper)
#    else
#        error architecture not supported
#    endif
        );

        __builtin_unreachable();
#else
#    error compiler not supported
#endif
    } else {
        std::longjmp(*fiber->context, 1);
    }
}


void
Scheduler::onFiberPreRun()
{
    if (deadFiber_ != nullptr) {
        destroyFiber(deadFiber_);
        deadFiber_ = nullptr;
    }

    if (currentFiber_->isPreInterrupted) {
        currentFiber_->isPreInterrupted = false;
        throw FiberInterruption();
    }
}


void
Scheduler::onFiberPostRun()
{
    if (currentFiber_->isPostInterrupted) {
        currentFiber_->isPostInterrupted = false;
        throw FiberInterruption();
    }
}


void
Scheduler::fiberStart() noexcept
{
    (currentFiber_->state = FiberState::Running, currentFiber_)->remove();
    Fiber *fiber;
    ++activeFiberCount_;

    try {
        onFiberPreRun();
        currentFiber_->procedure();
        onFiberPostRun();
        fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
    } catch (FiberInterruption) {
        fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
    } catch (...) {
        exception_ = std::current_exception();
        fiber = &idleFiber_;
    }

    --activeFiberCount_;
    deadFiber_ = currentFiber_;
    runFiber(fiber);
}

} // namespace siren
