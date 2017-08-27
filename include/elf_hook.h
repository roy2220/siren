#pragma once


#include <cstdint>
#include <algorithm>
#include <exception>
#include <type_traits>

#include "assert.h"


namespace siren {

template <class T>
class ELFHook final
{
public:
    inline explicit ELFHook(const char *, const char *, T *);

    inline ELFHook(ELFHook &&) noexcept;
    inline ELFHook &operator=(ELFHook &&) noexcept;

    inline void move(ELFHook *) noexcept;
    inline bool isValid() const noexcept;
    inline T *getCurrentFunction() const noexcept;
    inline T *getAlternateFunction() const noexcept;
    inline void toggle() noexcept;

private:
    T **currentFunction_;
    T *alternateFunction_;
};


class DLError final
  : public std::exception
{
public:
    inline const char *what() const noexcept override;

    explicit DLError() noexcept;

private:
    const char *description_;
};


template <class T>
inline ELFHook<T> MakeELFHook(const char *, const char *, T *);


namespace detail {

std::uintptr_t *LocateFunctionPointer(const char *, const char *);

} // namespace detail

} // namespace siren


/*
 * #include "elf_hook-inl.h"
 */


namespace siren {

template <class T>
ELFHook<T>::ELFHook(const char *elfFileName, const char *functionName, T *alternateFunction)
  : currentFunction_(reinterpret_cast<T **>(detail::LocateFunctionPointer(elfFileName
                                                                          , functionName))),
    alternateFunction_(alternateFunction)
{
    SIREN_ASSERT(alternateFunction != nullptr);
}


template <class T>
ELFHook<T>::ELFHook(ELFHook &&other) noexcept
{
    other.move(this);
}


template <class T>
ELFHook<T> &
ELFHook<T>::operator=(ELFHook &&other) noexcept
{
    if (&other != this) {
        other.move(this);
    }
}


template <class T>
void
ELFHook<T>::move(ELFHook *other) noexcept
{
    other->currentFunction_ = currentFunction_;
    other->alternateFunction_ = alternateFunction_;
    currentFunction_ = nullptr;
}


template <class T>
bool
ELFHook<T>::isValid() const noexcept
{
    return currentFunction_ != nullptr;
}


template <class T>
T *
ELFHook<T>::getCurrentFunction() const noexcept
{
    SIREN_ASSERT(isValid());
    return *currentFunction_;
}


template <class T>
T *
ELFHook<T>::getAlternateFunction() const noexcept
{
    SIREN_ASSERT(isValid());
    return alternateFunction_;
}


template <class T>
void
ELFHook<T>::toggle() noexcept
{
    SIREN_ASSERT(isValid());
    std::swap(*currentFunction_, alternateFunction_);
}


const char *
DLError::what() const noexcept
{
    return description_;
}


template <class T>
ELFHook<T>
MakeELFHook(const char *elfFileName, const char *functionName, T *alternateFunction)
{
    return ELFHook<T>(elfFileName, functionName, alternateFunction);
}

} // namespace siren
