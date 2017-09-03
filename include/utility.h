#pragma once


#include <cstddef>
#include <type_traits>
#include <utility>


#define SIREN_STR(X) \
    SIREN__STR_HELPER(X)

#define SIREN__STR_HELPER(X) \
    #X

#define SIREN_STRLEN(X) \
    (sizeof(X) - 1)

#define SIREN_CONCAT(X, Y) \
    SIREN__CONCAT_HELPER(X, Y)

#define SIREN__CONCAT_HELPER(X, Y) \
    X##Y

#define SIREN_UNUSED(X) \
    (static_cast<void>(X))

#define SIREN_TEST_INSTANTIATION(X, Y) \
    (siren::detail::TestInstantiationHelper<std::decay_t<X>, Y>::Result)


namespace siren {

inline std::size_t AlignSize(std::size_t, std::size_t) noexcept;

template <class T>
inline std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value
                        , T> NextPowerOfTwo(T) noexcept;

template <class T>
inline std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value
                        , bool> TestPowerOfTwo(T) noexcept;

template <class T>
inline std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value
                        , std::make_signed_t<T>> UnsignedToSigned(T) noexcept;

template <class T, class U>
inline std::enable_if_t<std::is_const<T>::value == std::is_const<U>::value
                        && (sizeof(T) == sizeof(U) && alignof(T) == alignof(U))
                        && (std::is_pod<T>::value && std::is_pod<U>::value), void>
    ConvertPointer(T *, U **) noexcept;

template <class T, class U>
inline decltype(auto) ApplyFunction(T &&, U &&);

template <class T, class U>
inline void IterateTuple(T &&, U &&);


namespace detail {

template <class T, template <class ...> class U>
struct TestInstantiationHelper
{
    static constexpr bool Result = false;
};


template <class ...T, template <class ...> class U>
struct TestInstantiationHelper<U<T ...>, U>
{
    static constexpr bool Result = true;
};


template <class T, class U, std::size_t ...N>
inline decltype(auto) ApplyFunctionHelper(T &&, U &&, std::index_sequence<N...>);

template <class T, class U, class ...V>
inline void IterateTupleHelper(T &&, U &&, V &&...);

template <class T>
inline void IterateTupleHelper(T &&);

} // namespace detail

} // namespace siren


/*
 * #include "utility-inl.h"
 */


#include <limits>
#include <tuple>


namespace siren {

std::size_t
AlignSize(std::size_t size, std::size_t alignment) noexcept
{
    return (size + alignment - 1) & -alignment;
}


template <class T>
std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, T>
NextPowerOfTwo(T x) noexcept
{
    constexpr unsigned int k = std::numeric_limits<T>::digits;

    --x;

    for (unsigned int n = 1; n < k; n *= 2) {
        x |= x >> n;
    }

    ++x;
    return x;
}


template <class T>
std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, bool>
TestPowerOfTwo(T x) noexcept
{
    return (x & (x - 1)) == 0;
}


template <class T>
std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, std::make_signed_t<T>>
UnsignedToSigned(T x) noexcept
{
    typedef std::make_signed_t<T> U;
    constexpr T k1 = std::numeric_limits<U>::max();
    constexpr T k2 = std::numeric_limits<T>::max();

    return x <= k1 ? static_cast<U>(x) : -static_cast<U>(k2 - x) - 1;
}


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


template <class T, class U>
decltype(auto)
ApplyFunction(T &&function, U &&arguments)
{
    constexpr std::size_t k = std::tuple_size<std::decay_t<U>>::value;

    return detail::ApplyFunctionHelper(std::forward<T>(function), std::forward<U>(arguments)
                                       , std::make_index_sequence<k>());
}


template <class T, class U>
void IterateTuple(T &&tuple, U &&callback)
{
    ApplyFunction([&] (auto &&...values) {
        detail::IterateTupleHelper(std::forward<U>(callback)
                                   , std::forward<decltype(values)>(values)...);
    }, std::forward<T>(tuple));
}


namespace detail {

template <class T, class U, std::size_t ...N>
decltype(auto)
ApplyFunctionHelper(T &&function, U &&arguments, std::index_sequence<N...>)
{
    return function(std::get<N>(std::forward<U>(arguments))...);
}


template <class T, class U, class ...V>
void
IterateTupleHelper(T &&callback, U &&value, V &&...values)
{
    callback(std::forward<U>(value));
    IterateTupleHelper(std::forward<T>(callback), std::forward<V>(values)...);
}


template <class T>
void
IterateTupleHelper(T &&callback)
{
    SIREN_UNUSED(callback);
}

} // namespace detail

} // namespace siren
