#pragma once


#include <type_traits>


namespace siren {

template <class T>
inline std::enable_if_t<std::is_unsigned<T>::value, T> NextPowerOfTwo(T);

}


/*
 * #include "next_power_of_two-inl.h"
 */


#include <limits>


namespace siren {

template <class T>
std::enable_if_t<std::is_unsigned<T>::value, T>
NextPowerOfTwo(T x)
{
    constexpr int k = std::numeric_limits<T>::digits;

    --x;

    for (int n = 1; n < k; n += n) {
        x |= x >> n;
    }

    ++x;
    return x;
}

}
