#pragma once


#include "config.h"
#include "macros.h"


#ifdef SIREN_WITH_DEBUG
#  define SIREN_ASSERT(EXPRESSION)                                                      \
    do {                                                                                \
        if (!(EXPRESSION)) {                                                            \
            ::siren::detail::AssertionFails(__FILE__, __LINE__, SIREN_STR(EXPRESSION)); \
        }                                                                               \
    } while (false)
#else
#  define SIREN_ASSERT(EXPRESSION) SIREN_UNUSED(0)
#endif


namespace siren {

namespace detail {

[[noreturn]] void AssertionFails(const char *, unsigned int, const char *) noexcept;

} // namespace detail

} // namespace siren
