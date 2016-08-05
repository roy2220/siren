#pragma once


#include <functional>
#include <utility>


namespace siren {

class ScopeGuard final
{
public:
    template <class T>
    inline explicit ScopeGuard(T &&);

    inline ~ScopeGuard();

    inline void commit() noexcept;
    inline void dismiss() noexcept;

private:
    std::function<void ()> rollback_;
    bool committed_;

    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;
};

}


/*
 * #include "scope_guard-inl.h"
 */


namespace siren {

template <class T>
ScopeGuard::ScopeGuard(T &&rollback)
  : rollback_(std::forward<T>(rollback)),
    committed_(false)
{
}


ScopeGuard::~ScopeGuard()
{
    if (committed_) {
        rollback_();
    }
}


void
ScopeGuard::commit() noexcept
{
    committed_ = true;
}


void
ScopeGuard::dismiss() noexcept
{
    committed_ = false;
}

}
