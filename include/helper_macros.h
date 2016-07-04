#pragma once


#include <cstddef>


#define STR_IMPL(X) \
    #X

#define STR(X) \
    STR_IMPL(X)

#define CONCAT_IMPL(A, B) \
    A##B

#define CONCAT(A, B) \
    CONCAT_IMPL(A, B)

#define SSIZE_OF(X) \
    (static_cast<std::ptrdiff_t>(sizeof(X)))
