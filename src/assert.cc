#include "assert.h"


#include <cstdio>
#include <exception>

#include "stack_trace.h"


namespace siren {

namespace detail {

void
AssertionFails(const char *fileName, unsigned int lineNumber, const char *expression) noexcept
{
    std::fprintf(stderr, "%s:%u: Assert `%s` failed\n", fileName, lineNumber, expression);
    std::fputs("Stack trace: \n", stderr);

    ExtractStackTrace([] (const char *executableFileName, void *instruction) -> void {
        std::fprintf(stderr, "  %s[%p]\n", executableFileName, instruction);
    });

    std::terminate();
}

} // namespace detail

} // namespace siren
