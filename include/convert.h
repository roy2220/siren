#pragma once


#include <type_traits>


namespace siren {

template <class T, class U>
inline std::enable_if_t<std::is_pod<T>::value && std::is_pod<U>::value && sizeof(U) == sizeof(T)
                        && alignof(U) == alignof(T), void>
    Convert(const T &, U *) noexcept;

} // namespace siren


/*
 * #include "unsigned_to_signed-inl.h"
 */


namespace siren {

template <class T, class U>
std::enable_if_t<std::is_pod<T>::value && std::is_pod<U>::value && sizeof(U) == sizeof(T)
                 && alignof(U) == alignof(T), void>
Convert(const T &input, U *output) noexcept
{
    union {
        T from;
        U to;
    } conversion;

    conversion.from = input;
    *output = conversion.to;
}

} // namespace siren
