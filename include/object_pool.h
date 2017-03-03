#pragma once


#include <cstddef>

#include "config.h"
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

    template <class ...U>
    inline T *createObject(U &&...);

private:
    std::size_t memoryBlockAlignment_;
    MemoryPool memoryPool_;
#ifdef SIREN_WITH_DEBUG
    std::size_t objectCount_;
#endif
};

} // namespace siren


/*
 * #include "object_pool-inl.h"
 */


#include <algorithm>
#include <new>
#include <utility>

#include "assert.h"
#include "scope_guard.h"
#include "utility.h"


namespace siren {

template <class T>
ObjectPool<T>::ObjectPool(std::size_t numberOfObjectsToReserve, std::size_t objectTagAlignment
                          , std::size_t objectTagSize) noexcept
  : memoryBlockAlignment_(std::max(alignof(T), NextPowerOfTwo(objectTagAlignment))),
    memoryPool_(memoryBlockAlignment_, AlignSize(sizeof(T), memoryBlockAlignment_)
                                       + AlignSize(objectTagSize, memoryBlockAlignment_)
                , numberOfObjectsToReserve)
#ifdef SIREN_WITH_DEBUG
        ,
    objectCount_(0)
#endif
{
}


template <class T>
ObjectPool<T>::~ObjectPool()
{
    SIREN_ASSERT(objectCount_ == 0);
}


template <class T>
template <class ...U>
T *
ObjectPool<T>::createObject(U &&...argument)
{
    void *memoryBlock = memoryPool_.allocateBlock();

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        memoryPool_.freeBlock(memoryBlock);
    });

    T *object = new (memoryBlock) T(std::forward<U>(argument)...);
    scopeGuard.dismiss();
#ifdef SIREN_WITH_DEBUG
    ++objectCount_;
#endif
    return object;
}


template <class T>
void
ObjectPool<T>::destroyObject(T *object) noexcept
{
    SIREN_ASSERT(objectCount_ >= 1);
    SIREN_ASSERT(object != nullptr);
    void *memoryBlock = (object->~T(), object);
    memoryPool_.freeBlock(memoryBlock);
#ifdef SIREN_WITH_DEBUG
    --objectCount_;
#endif
}


template <class T>
const void *
ObjectPool<T>::getObjectTag(const T *object) const noexcept
{
    SIREN_ASSERT(object != nullptr);
    return reinterpret_cast<const char *>(object) + AlignSize(sizeof(T), memoryBlockAlignment_);
}


template <class T>
void *
ObjectPool<T>::getObjectTag(T *object) const noexcept
{
    SIREN_ASSERT(object != nullptr);
    return reinterpret_cast<char *>(object) + AlignSize(sizeof(T), memoryBlockAlignment_);
}

} // namespace siren
