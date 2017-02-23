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
    const std::size_t minChunkSize_;
    std::vector<void *> chunks_;
    std::size_t newChunkSize_;
    void *lastNewBlock_;
    void *firstNewBlock_;
    void *lastFreeBlock_;

    inline void *GetBlockNext(void *) noexcept;
    inline void SetBlockNext(void *, void *) noexcept;

    void initialize() noexcept;
    void finalize() noexcept;
    void move(MemoryPool *) noexcept;
    void *makeBlock();
};

} // namespace siren


/*
 * #include "memory_pool-inl.h"
 */


#include <cassert>
#include <cstring>


namespace siren {

void *
MemoryPool::GetBlockNext(void *block) noexcept
{
    void *blockNext;
    std::memcpy(&blockNext, block, sizeof(blockNext));
    return blockNext;
}


void
MemoryPool::SetBlockNext(void *block, void *blockNext) noexcept
{
    std::memcpy(block, &blockNext, sizeof(blockNext));
}


void *
MemoryPool::allocateBlock()
{
    if (lastFreeBlock_ == nullptr) {
        return makeBlock();
    } else {
        void *block = lastFreeBlock_;
        lastFreeBlock_ = GetBlockNext(block);
        return block;
    }
}


void
MemoryPool::freeBlock(void *block) noexcept
{
    assert(block != nullptr);
    SetBlockNext(block, lastFreeBlock_);
    lastFreeBlock_ = block;
}

} // namespace siren
