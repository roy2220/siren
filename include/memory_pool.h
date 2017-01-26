#pragma once


#include <cstddef>
#include <vector>


namespace siren {

class MemoryPool final
{
public:
    inline void *allocateBlock();
    inline void freeBlock(void *) noexcept;

    explicit MemoryPool(std::size_t, std::size_t, std::size_t = 0) noexcept;
    MemoryPool(MemoryPool &&) noexcept;
    ~MemoryPool();
    MemoryPool &operator=(MemoryPool &&) noexcept;

    void reset() noexcept;

private:
    const std::size_t blockAlignment_;
    const std::size_t blockSize_;
    const std::size_t firstChunkSize_;
    std::size_t chunkSize_;
    std::vector<void *> chunks_;
    void *lastFreeBlock_;

    void initialize() noexcept;
    void finalize() noexcept;
    void move(MemoryPool *) noexcept;
    void makeFreeBlocks();
};

} // namespace siren


/*
 * #include "memory_pool-inl.h"
 */


#include <cassert>


namespace siren {

void *
MemoryPool::allocateBlock()
{
    if (lastFreeBlock_ == nullptr) {
        makeFreeBlocks();
    }

    void *block = lastFreeBlock_;
    lastFreeBlock_ = *static_cast<void **>(block);
    return block;
}


void
MemoryPool::freeBlock(void *block) noexcept
{
    assert(block != nullptr);
    *static_cast<void **>(block) = lastFreeBlock_;
    lastFreeBlock_ = block;
}

} // namespace siren
