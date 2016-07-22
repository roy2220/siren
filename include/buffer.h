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
    inline operator const T *() const;
    inline operator T *();

    inline std::size_t getLength() const;
    inline void setLength(std::size_t);

private:
    T *base_;
    std::size_t length_;

    inline void finalize();
    inline void initialize();

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
    other.initialize();
}


template <class T>
Buffer<T, true>::~Buffer()
{
    finalize();
}


template <class T>
Buffer<T, true> &
Buffer<T, true>::operator=(Buffer &&other)
{
    if (&other != this) {
        finalize();
        base_ = other.base_;
        length_ = other.length_;
        other.initialize();
    }

    return *this;
}


template <class T>
Buffer<T, true>::operator const T *() const
{
    return base_;
}


template <class T>
Buffer<T, true>::operator T *()
{
    return base_;
}


template <class T>
void
Buffer<T, true>::finalize()
{
    std::free(base_);
}


template <class T>
void
Buffer<T, true>::initialize()
{
    base_ = nullptr;
    length_ = 0;
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
