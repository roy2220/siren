#pragma once


#include <cstddef>

#include "memory_pool.h"


namespace siren {

template <class T>
class ObjectPool final
{
public:
    inline explicit ObjectPool(std::size_t = 0, std::size_t = 0, std::size_t = 0) noexcept;
    inline ~ObjectPool();

    ObjectPool(ObjectPool &&) noexcept = default;
    ObjectPool &operator=(ObjectPool &&) noexcept = default;

    inline void destroyObject(T *) noexcept;
    inline const void *getObjectTag(const T *) const noexcept;
    inline void *getObjectTag(T *) const noexcept;

    template <class ...Args>
    inline T *createObject(Args &&...);

private:
    std::size_t memoryBlockAlignment_;
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
#include <algorithm>
#include <utility>

#include "helper_macros.h"
#include "next_power_of_two.h"
#include "scope_guard.h"


namespace siren {

template <class T>
ObjectPool<T>::ObjectPool(std::size_t numberOfObjectsToReserve, std::size_t objectTagAlignment
                          , std::size_t objectTagSize) noexcept
  : memoryBlockAlignment_(std::max(alignof(T), NextPowerOfTwo(objectTagAlignment))),
    memoryPool_(memoryBlockAlignment_, SIREN_ALIGN(sizeof(T), memoryBlockAlignment_)
                                       + SIREN_ALIGN(objectTagSize, memoryBlockAlignment_)
                , numberOfObjectsToReserve)
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


template <class T>
const void *
ObjectPool<T>::getObjectTag(const T *object) const noexcept
{
    assert(object != nullptr);
    return reinterpret_cast<const char *>(object) + SIREN_ALIGN(sizeof(T), memoryBlockAlignment_);
}


template <class T>
void *
ObjectPool<T>::getObjectTag(T *object) const noexcept
{
    assert(object != nullptr);
    return reinterpret_cast<char *>(object) + SIREN_ALIGN(sizeof(T), memoryBlockAlignment_);
}

} // namespace siren
