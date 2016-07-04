#pragma once


#define STR_IMPL(X) \
    #X

#define STR(X) \
    STR_IMPL(X)

#define CONCAT_IMPL(A, B) \
    A##B

#define CONCAT(A, B) \
    CONCAT_IMPL(A, B)

#define UNIQUE_NAME(PREFIX) \
    CONCAT(PREFIX, __LINE__)
