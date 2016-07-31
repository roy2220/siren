#pragma once


#include <cstddef>
#include <type_traits>


namespace siren {

template <class T, bool = std::is_pod<T>::value>
class Buffer;


template <class T>
class Buffer<T, true> final
{
public:
    inline explicit Buffer() noexcept;
    inline Buffer(Buffer &&) noexcept;
    inline ~Buffer();
    inline Buffer &operator=(Buffer &&) noexcept;
    inline operator const T *() const noexcept;
    inline operator T *() noexcept;

    inline void reset() noexcept;
    inline std::size_t getLength() const noexcept;
    inline void setLength(std::size_t);

private:
    T *base_;
    std::size_t length_;

    inline void initialize() noexcept;
    inline void finalize() noexcept;
    inline void move(Buffer *) noexcept;
};

}


/*
 * #include "buffer-inl.h"
 */


#include <cerrno>
#include <cstdlib>
#include <system_error>

#include "next_power_of_two.h"


namespace siren {

template <class T>
Buffer<T, true>::Buffer() noexcept
{
    initialize();
}


template <class T>
Buffer<T, true>::Buffer(Buffer &&other) noexcept
{
    other.move(this);
}


template <class T>
Buffer<T, true>::~Buffer()
{
    finalize();
}


template <class T>
Buffer<T, true> &
Buffer<T, true>::operator=(Buffer &&other) noexcept
{
    if (&other != this) {
        finalize();
        other.move(this);
    }

    return *this;
}


template <class T>
Buffer<T, true>::operator const T *() const noexcept
{
    return base_;
}


template <class T>
Buffer<T, true>::operator T *() noexcept
{
    return base_;
}


template <class T>
void
Buffer<T, true>::initialize() noexcept
{
    base_ = nullptr;
    length_ = 0;
}


template <class T>
void
Buffer<T, true>::finalize() noexcept
{
    std::free(base_);
}


template <class T>
void
Buffer<T, true>::move(Buffer *other) noexcept
{
    other->base_ = base_;
    other->length_ = length_;
    initialize();
}


template <class T>
void
Buffer<T, true>::reset() noexcept
{
    finalize();
    initialize();
}


template <class T>
std::size_t
Buffer<T, true>::getLength() const noexcept
{
    return length_;
}


template <class T>
void
Buffer<T, true>::setLength(std::size_t length)
{
    std::size_t size = NextPowerOfTwo(length * sizeof(T));
    T *base;

    if (size == 0) {
        base = (std::free(base_), nullptr);
    } else {
        base = static_cast<T *>(std::realloc(base_, size));

        if (base == nullptr) {
            throw std::system_error(errno, std::system_category(), "realloc() failed");
        }
    }

    base_ = base;
    length_ = size / sizeof(T);
}

}
