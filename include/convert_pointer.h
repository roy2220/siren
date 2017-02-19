#pragma once


#include <type_traits>


namespace siren {

template <class T, class U>
inline std::enable_if_t<std::is_const<T>::value == std::is_const<U>::value
                        && (sizeof(T) == sizeof(U) && alignof(T) == alignof(U))
                        && (std::is_pod<T>::value && std::is_pod<U>::value), void>
    ConvertPointer(T *, U **) noexcept;

} // namespace siren


/*
 * #include "convert_pointer-inl.h"
 */


namespace siren {

template <class T, class U>
std::enable_if_t<std::is_const<T>::value == std::is_const<U>::value && (sizeof(T) == sizeof(U)
                                                                        && alignof(T) == alignof(U))
                 && (std::is_pod<T>::value && std::is_pod<U>::value), void>
ConvertPointer(T *input, U **output) noexcept
{
    union {
        T *from;
        U *to;
    } conversion;

    conversion.from = input;
    *output = conversion.to;
}

} // namespace siren
