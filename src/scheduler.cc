#include "scheduler.h"

#include <cerrno>
#include <cstdlib>
#include <system_error>


namespace siren {

void
Scheduler::FiberStart(Fiber *fiber) noexcept
{
    fiber->routine();
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
    fiber->~Fiber();
    std::free(base);
}


void
Scheduler::switchToFiber(Fiber *fiber) noexcept
{
    std::jmp_buf context;

    if (setjmp(context) == 0) {
        runningFiber_->context = &context;
        runFiber(fiber);
    } else {
        if (fiberToFree_ != nullptr) {
            freeFiber(fiberToFree_);
            fiberToFree_ = nullptr;
        }
    }
}


void
Scheduler::runFiber(Fiber *fiber) noexcept
{
    runningFiber_ = fiber;

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
            : "r"(fiber->stack + fiber->stackSize), "r"(fiber), "r"(FiberStart)
#    elif defined(__x86_64__)
            "movq\t$0, %%rbp\n\t"
            "movq\t%0, %%rsp\n\t"
            "pushq\t$0\n\t"
            "jmpq\t*%2"
            :
            : "r"(fiber->stack + fiber->stackSize), "D"(fiber), "r"(FiberStart)
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

}
