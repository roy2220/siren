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
    inline explicit Buffer();
    inline Buffer(Buffer &&);
    inline ~Buffer();
    inline Buffer &operator=(Buffer &&);

    inline const T *get() const;
    inline T *get();
    inline std::size_t getLength() const;
    inline void setLength(std::size_t);

private:
    T *base_;
    std::size_t length_;

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;
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
Buffer<T, true>::Buffer()
  : base_(nullptr),
    length_(0)
{
}


template <class T>
Buffer<T, true>::Buffer(Buffer &&other)
  : base_(other.base_),
    length_(other.length_)
{
    other.base_ = nullptr;
    other.length_ = 0;
}


template <class T>
Buffer<T, true>::~Buffer()
{
    std::free(base_);
}


template <class T>
Buffer<T, true> &
Buffer<T, true>::operator=(Buffer &&other)
{
    if (&other != this) {
        std::free(base_);
        base_ = other.base_;
        length_ = other.length_;
        other.base_ = nullptr;
        other.length_ = 0;
    }

    return *this;
}


template <class T>
const T *
Buffer<T, true>::get() const
{
    return base_;
}


template <class T>
T *
Buffer<T, true>::get()
{
    return base_;
}


template <class T>
std::size_t
Buffer<T, true>::getLength() const
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
