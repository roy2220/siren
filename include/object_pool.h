#pragma once


#include <cstddef>

#include "memory_pool.h"


namespace siren {

template <class T>
class ObjectPool final
{
public:
    inline explicit ObjectPool(std::size_t = 0) noexcept;
    inline ~ObjectPool();

    ObjectPool(ObjectPool &&) noexcept = default;
    ObjectPool &operator=(ObjectPool &&) noexcept = default;

    inline void destroyObject(T *) noexcept;

    template <class ...Args>
    inline T *createObject(Args &&...);

private:
    MemoryPool memoryPool_;
#ifndef NDEBUG
    std::size_t objectCount_;
#endif
};

} // namespace siren


/*
 * #include "object_pool-inl.h"
 */


#include <cassert>
#include <utility>

#include "scope_guard.h"


namespace siren {

template <class T>
ObjectPool<T>::ObjectPool(std::size_t numberOfObjectsToReserve) noexcept
  : memoryPool_(alignof(T), sizeof(T), numberOfObjectsToReserve)
#ifndef NDEBUG
        ,
    objectCount_(0)
#endif
{
}


template <class T>
ObjectPool<T>::~ObjectPool()
{
    assert(objectCount_ == 0);
}


template <class T>
template <class ...Args>
T *
ObjectPool<T>::createObject(Args &&...args)
{
    void *memoryBlock = memoryPool_.allocateBlock();

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        memoryPool_.freeBlock(memoryBlock);
    });

    T *object = new (memoryBlock) T(std::forward<Args>(args)...);
    scopeGuard.dismiss();
#ifndef NDEBUG
    ++objectCount_;
#endif
    return object;
}


template <class T>
void
ObjectPool<T>::destroyObject(T *object) noexcept
{
    assert(objectCount_ >= 1);
    assert(object != nullptr);
    void *memoryBlock = (object->~T(), object);
    memoryPool_.freeBlock(memoryBlock);
#ifndef NDEBUG
    --objectCount_;
#endif
}

} // namespace siren
