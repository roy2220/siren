#pragma once


#include <type_traits>


namespace siren {

template <class T>
inline std::enable_if_t<std::is_unsigned<T>::value, std::make_signed_t<T>> UnsignedToSigned(T);

}


/*
 * #include "unsigned_to_signed-inl.h"
 */


#include <limits>


namespace siren {

template <class T>
std::enable_if_t<std::is_unsigned<T>::value, std::make_signed_t<T>>
UnsignedToSigned(T x)
{
    using U = std::make_signed_t<T>;
    constexpr T k1 = std::numeric_limits<U>::max();
    constexpr T k2 = std::numeric_limits<T>::max();

    return x <= k1 ? x : -static_cast<U>(k2 - x) - 1;
}

}
