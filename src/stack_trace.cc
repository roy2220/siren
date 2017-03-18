#include "stack_trace.h"

#include <cstring>
#include <cxxabi.h>

#include "scheduler.h"


namespace siren {

namespace detail {

thread_local void *StackTraceOfLastThrow[SIREN__MAX_STACK_DEPTH + 1];

} // namespace detail


namespace {

inline void CaptureStackTraceOfLastThrow() noexcept;


void
CaptureStackTraceOfLastThrow() noexcept
{
    int stackDepth = backtrace(detail::StackTraceOfLastThrow, SIREN__MAX_STACK_DEPTH);
    detail::StackTraceOfLastThrow[stackDepth] = nullptr;
}

} // namespace

} // namespace siren


extern "C" {

void
__cxa_throw(void *arg1, std::type_info *typeID, void (*arg3)(void *))
{
    if (std::strcmp(typeID->name(), typeid(siren::FiberInterruption).name()) != 0) {
        siren::CaptureStackTraceOfLastThrow();
    }

    static const auto real_cxa_throw
                      = reinterpret_cast<decltype(&__cxa_throw)>(dlsym(RTLD_NEXT, "__cxa_throw"));
    real_cxa_throw(arg1, typeID, arg3);
}

} // extern "C"
