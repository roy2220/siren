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
    std::size_t nextChunkSize_;
    std::vector<void *> chunks_;
    void *lastBlock_;

    void initialize() noexcept;
    void finalize() noexcept;
    void move(MemoryPool *) noexcept;
    void makeBlocks();
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
    if (lastBlock_ == nullptr) {
        makeBlocks();
    }

    void *block = lastBlock_;
    lastBlock_ = *static_cast<void **>(block);
    return block;
}


void
MemoryPool::freeBlock(void *block) noexcept
{
    assert(block != nullptr);
    *static_cast<void **>(block) = lastBlock_;
    lastBlock_ = block;
}

} // namespace siren
