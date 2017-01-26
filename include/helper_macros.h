#pragma once


#define SIREN__STR(X) \
    #X

#define SIREN_STR(X) \
    SIREN__STR(X)

#define SIREN__CONCAT(X, Y) \
    X##Y

#define SIREN_CONCAT(X, Y) \
    SIREN__CONCAT(X, Y)

#define SIREN_ALIGN(X, Y) \
    (((X) + (Y) - 1) & ~((Y) - 1))

#define SIREN_UNUSED(X) \
    ((void)(X))
