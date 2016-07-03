#pragma once


#define STR_IMPL_(X) \
    #X

#define STR(X) \
    STR_IMPL_(X)

#define CONCAT_IMPL_(A, B) \
    A##B

#define CONCAT(A, B) \
    CONCAT_IMPL_(A, B)

#define UNIQUE_ID(ID_PREFIX) \
    CONCAT(ID_PREFIX, __LINE__)
