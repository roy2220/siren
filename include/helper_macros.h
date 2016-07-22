#pragma once


#define SIREN_STR_IMPL(X) \
    #X

#define SIREN_STR(X) \
    SIREN_STR_IMPL(X)

#define SIREN_CONCAT_IMPL(X, Y) \
    X##Y

#define SIREN_CONCAT(X, Y) \
    SIREN_CONCAT_IMPL(X, Y)

#define SIREN_ALIGN(X, Y) \
    (((X) + (Y) - 1) & ~((Y) - 1))
