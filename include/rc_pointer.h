#pragma once


namespace siren {

template <class T>
class RCPointer final
{
public:
    inline explicit RCPointer(T * = nullptr) noexcept;
    inline RCPointer(const RCPointer &) noexcept;
    inline RCPointer(RCPointer &&) noexcept;
    inline ~RCPointer();
    inline RCPointer &operator=(const RCPointer &) noexcept;
    inline RCPointer &operator=(RCPointer &&) noexcept;
    inline T &operator*() const noexcept;
    inline T *operator->() const noexcept;

    inline T *get() const noexcept;
    inline void reset() noexcept;

private:
    T *object_;

    inline void initialize(T * = nullptr) noexcept;
    inline void finalize() noexcept;
    inline void copy(RCPointer *) const noexcept;
    inline void move(RCPointer *) noexcept;
};

} // namespace siren


/*
 * #include "rc_pointer-inl.h"
 */


#include <cassert>


namespace siren {

template <class T>
RCPointer<T>::RCPointer(T *object) noexcept
{
    initialize(object);
}


template <class T>
RCPointer<T>::RCPointer(const RCPointer &other) noexcept
{
    other.copy(this);
}


template <class T>
RCPointer<T>::RCPointer(RCPointer &&other) noexcept
{
    other.move(this);
}


template <class T>
RCPointer<T>::~RCPointer()
{
    finalize();
}


template <class T>
RCPointer<T> &
RCPointer<T>::operator=(const RCPointer &other) noexcept
{
    if (&other != this) {
        finalize();
        other.copy(this);
    }

    return *this;
}


template <class T>
RCPointer<T> &
RCPointer<T>::operator=(RCPointer &&other) noexcept
{
    if (&other != this) {
        finalize();
        other.move(this);
    }

    return *this;
}


template <class T>
T &
RCPointer<T>::operator*() const noexcept
{
    assert(object_ != nullptr);
    return *object_;
}


template <class T>
T *
RCPointer<T>::operator->() const noexcept
{
    return object_;
}


template <class T>
T *
RCPointer<T>::get() const noexcept
{
    return object_;
}


template <class T>
void
RCPointer<T>::reset() noexcept
{
    finalize();
    initialize();
}


template <class T>
void
RCPointer<T>::initialize(T *object) noexcept
{
    object_ = object;
}


template <class T>
void
RCPointer<T>::finalize() noexcept
{
    if (object_ != nullptr) {
        ReleaseObject(object_);
    }
}


template <class T>
void
RCPointer<T>::copy(RCPointer *other) const noexcept
{
    other->object_ = object_;

    if (object_ != nullptr) {
        AddObjectRef(object_);
    }
}


template <class T>
void
RCPointer<T>::move(RCPointer *other) noexcept
{
    other->object_ = object_;
    initialize();
}

} // namespace siren
