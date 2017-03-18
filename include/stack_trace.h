#pragma once


namespace siren {

template <class T>
inline void ExtractStackTrace(T &&);

template <class T>
inline void ExtractStackTraceOfLastThrow(T &&);


namespace detail {

extern thread_local void *StackTraceOfLastThrow[];


inline void **CaptureStackTrace() noexcept;
inline const char *GetExecutableFileName(void *);

} // namespace detail

} // namespace siren


/*
 * #include "stack_trace.h"
 */


#include <dlfcn.h>
#include <execinfo.h>


#define SIREN__MAX_STACK_DEPTH 64


namespace siren {

template <class T>
void
ExtractStackTrace(T &&callback)
{
    for (void **instruction = detail::CaptureStackTrace() + 1; *instruction != nullptr
         ; ++instruction) {
        const char *executableFileName = detail::GetExecutableFileName(*instruction);
        callback(executableFileName, *instruction);
    }
}


template <class T>
void
ExtractStackTraceOfLastThrow(T &&callback)
{
    for (void **instruction = detail::StackTraceOfLastThrow + 1; *instruction != nullptr
         ; ++instruction) {
        const char *executableFileName = detail::GetExecutableFileName(*instruction);
        callback(executableFileName, *instruction);
    }
}


namespace detail {

void **
CaptureStackTrace() noexcept
{
    static thread_local void *stackTrace[SIREN__MAX_STACK_DEPTH + 1];
    int stackDepth = backtrace(stackTrace, SIREN__MAX_STACK_DEPTH);
    stackTrace[stackDepth] = nullptr;
    return stackTrace;
}


const char *
GetExecutableFileName(void *instruction)
{
    Dl_info info;
    dladdr(instruction, &info);
    return info.dli_fname;
}

} // namespace detail

} // namespace siren
