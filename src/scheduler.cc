#include "scheduler.h"

#include <cerrno>
#include <cstdlib>
#include <system_error>

#ifdef USE_VALGRIND
#    include <valgrind/valgrind.h>
#endif


namespace siren {

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
Scheduler::switchToFiber(Fiber *fiber)
{
    std::jmp_buf context;

    if (setjmp(context) == 0) {
        runningFiber_->context = &context;
        runFiber(fiber);
    } else {
        onFiberRun();
    }
}


void
Scheduler::runFiber(Fiber *fiber) noexcept
{
    runningFiber_ = (fiber->state = FiberState::Running, fiber->remove(), fiber);

    if (fiber->context == nullptr) {
        void (Scheduler::*fiberStart)() = &Scheduler::fiberStart;

#if defined(__GNUG__)
        __asm__ __volatile__ (
#    if defined(__i386__)
            "movl\t$0, %%ebp\n\t"
            "movl\t%0, %%esp\n\t"
            "pushl\t%1\n\t"
            "pushl\t$0\n\t"
            "jmpl\t*%2"
            :
            : "r"(fiber->stack + fiber->stackSize), "r"(this), "r"(fiberStart)
#    elif defined(__x86_64__)
            "movq\t$0, %%rbp\n\t"
            "movq\t%0, %%rsp\n\t"
            "pushq\t$0\n\t"
            "jmpq\t*%2"
            :
            : "r"(fiber->stack + fiber->stackSize), "D"(this), "r"(fiberStart)
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
Scheduler::onFiberRun()
{
    if (deadFiber_ != nullptr) {
        destroyFiber(deadFiber_);
        deadFiber_ = nullptr;
    }

    if (runningFiber_->isInterrupted) {
        runningFiber_->isInterrupted = false;
        throw FiberInterruption();
    }
}


void
Scheduler::fiberStart() noexcept
{
    Fiber *fiber;
    ++activeFiberCount_;

    try {
        onFiberRun();
        runningFiber_->procedure();
        fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
    } catch (const FiberInterruption &) {
        fiber = static_cast<Fiber *>(runnableFiberList_.getTail());
    } catch (...) {
        exception_ = std::current_exception();
        fiber = &idleFiber_;
    }

    --activeFiberCount_;
    deadFiber_ = runningFiber_;
    runFiber(fiber);
}

}
