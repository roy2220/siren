#pragma once


#include <type_traits>


#define SIREN_ENUM_CLASS_AS_FLAG(T)                                   \
    inline T operator~(T x) {                                         \
        typedef std::underlying_type_t<T> U;                          \
        return static_cast<T>(~static_cast<U>(x));                    \
    }                                                                 \
                                                                      \
    inline T operator&(T x, T y) {                                    \
        typedef std::underlying_type_t<T> U;                          \
        return static_cast<T>(static_cast<U>(x) & static_cast<U>(y)); \
    }                                                                 \
                                                                      \
    inline T operator|(T x, T y) {                                    \
        typedef std::underlying_type_t<T> U;                          \
        return static_cast<T>(static_cast<U>(x) | static_cast<U>(y)); \
    }                                                                 \
                                                                      \
    inline T operator^(T x, T y) {                                    \
        typedef std::underlying_type_t<T> U;                          \
        return static_cast<T>(static_cast<U>(x) ^ static_cast<U>(y)); \
    }                                                                 \
                                                                      \
    inline T &operator&=(T &x, T y) {                                 \
        x = x & y;                                                    \
        return x;                                                     \
    }                                                                 \
                                                                      \
    inline T &operator|=(T &x, T y) {                                 \
        x = x | y;                                                    \
        return x;                                                     \
    }                                                                 \
                                                                      \
    inline T &operator^=(T &x, T y) {                                 \
        x = x ^ y;                                                    \
        return x;                                                     \
    }
