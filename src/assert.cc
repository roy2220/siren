#include "assert.h"


#include <cstdio>
#include <exception>


namespace siren {

namespace detail {

void
AssertionFails(const char *fileName, unsigned int lineNumber, const char *expression) noexcept
{
    std::fprintf(stderr, "Siren: %s:%u: Assert `%s` failed\n", fileName, lineNumber, expression);
    std::terminate();
}

} // namespace detail

} // namespace siren
