#pragma once


#define STR_IMPL(A) \
    #A

#define STR(A) \
    STR_IMPL(A)

#define CONCAT_IMPL(A, B) \
    A##B

#define CONCAT(A, B) \
    CONCAT_IMPL(A, B)

#define UNIQUE_ID(ID_PREFIX) \
    CONCAT(ID_PREFIX, __LINE__)
