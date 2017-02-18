#pragma once


#include <cstddef>


namespace siren {

class RCRecord;


template <class T>
class RCPointer final
{
public:
    inline explicit RCPointer(T * = nullptr) noexcept;
    inline RCPointer(const RCPointer &) noexcept;
    inline RCPointer(RCPointer &&) noexcept;
    inline ~RCPointer();
    inline RCPointer &operator=(T *) noexcept;
    inline RCPointer &operator=(const RCPointer &) noexcept;
    inline RCPointer &operator=(RCPointer &&) noexcept;
    inline T &operator*() const noexcept;
    inline T *operator->() const noexcept;

    inline T *get() const noexcept;
    inline void reset() noexcept;

private:
    typedef RCRecord Record;

    T *object_;

    inline void initialize(T * = nullptr) noexcept;
    inline void finalize() noexcept;
    inline void copy(RCPointer *) const noexcept;
    inline void move(RCPointer *) noexcept;
};


class RCRecord
{
protected:
    inline explicit RCRecord() noexcept;
    inline ~RCRecord();

private:
    std::size_t value_;

    RCRecord(const RCRecord &) = delete;
    RCRecord &operator=(const RCRecord &) = delete;

    template <class T>
    friend class RCPointer;
};


namespace detail {

template <class T>
void DestroyObject(T *) noexcept;

} // namespace detail

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
RCPointer<T>::operator=(T *object) noexcept
{
    if (object != object_) {
        finalize();
        initialize(object);
    }

    return *this;
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
RCPointer<T>::initialize(T *object) noexcept
{
    object_ = object;

    if (object != nullptr) {
        Record *record = object;
        ++record->value_;
    }
}


template <class T>
void
RCPointer<T>::finalize() noexcept
{
    if (object_ != nullptr) {
        Record *record = object_;

        if (record->value_-- == 1) {
            detail::DestroyObject<T>(object_);
        }
    }
}


template <class T>
void
RCPointer<T>::copy(RCPointer *other) const noexcept
{
    other->initialize(object_);
}


template <class T>
void
RCPointer<T>::move(RCPointer *other) noexcept
{
    other->object_ = object_;
    initialize();
}


template <class T>
void
RCPointer<T>::reset() noexcept
{
    finalize();
    initialize();
}


RCRecord::RCRecord() noexcept
  : value_(0)
{
}


RCRecord::~RCRecord()
{
    assert(value_ == 0);
}

} // namespace siren
