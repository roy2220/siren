#pragma once


#include <type_traits>


namespace siren {

template <class T, bool = std::is_nothrow_move_constructible<T>::value>
class ScopeGuard;


template <class T>
class ScopeGuard<T, true> final
{
public:
    inline explicit ScopeGuard(T &&) noexcept;
    inline ScopeGuard(ScopeGuard &&) noexcept;
    inline ~ScopeGuard();
    inline ScopeGuard &operator=(ScopeGuard &&) noexcept;

    inline void dismiss() noexcept;

private:
    bool isEngaged_;
    T rollback_;

    inline void initialize() noexcept;
    inline void finalize();
    inline void move(ScopeGuard *) noexcept;
};


template <class T>
inline ScopeGuard<std::remove_reference_t<T>> MakeScopeGuard(T &&) noexcept;

}


/*
 * #include "scope_guard-inl.h"
 */


#include <cassert>
#include <utility>


namespace siren {

template <class T>
ScopeGuard<T, true>::ScopeGuard(T &&rollback) noexcept
  : rollback_(std::move(rollback))
{
    initialize();
}


template <class T>
ScopeGuard<T, true>::ScopeGuard(ScopeGuard &&other) noexcept
  : rollback_(std::move(other.rollback_))
{
    other.move(this);
}


template <class T>
ScopeGuard<T, true>::~ScopeGuard()
{
    finalize();
}


template <class T>
ScopeGuard<T, true> &
ScopeGuard<T, true>::operator=(ScopeGuard &&other) noexcept
{
    if (&other != this) {
        finalize();
        rollback_ = std::move(other.rollback_);
        other.move(this);
    }

    return *this;
}


template <class T>
void
ScopeGuard<T, true>::initialize() noexcept
{
    isEngaged_ = true;
}


template <class T>
void
ScopeGuard<T, true>::finalize()
{
    if (isEngaged_) {
        rollback_();
    }
}


template <class T>
void
ScopeGuard<T, true>::move(ScopeGuard *other) noexcept
{
    other->isEngaged_ = isEngaged_;
    isEngaged_ = false;
}


template <class T>
void
ScopeGuard<T, true>::dismiss() noexcept
{
    assert(isEngaged_);
    isEngaged_ = false;
}


template <class T>
ScopeGuard<std::remove_reference_t<T>>
MakeScopeGuard(T &&rollback) noexcept
{
    typedef std::remove_reference_t<T> U;

    return ScopeGuard<U>(std::forward<T>(rollback));
}

}
