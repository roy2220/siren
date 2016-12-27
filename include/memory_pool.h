#pragma once


#include <cstddef>
#include <vector>


namespace siren {

class MemoryPool final
{
public:
    inline explicit MemoryPool(std::size_t, std::size_t, std::size_t = 0) noexcept;
    inline MemoryPool(MemoryPool &&) noexcept;
    inline ~MemoryPool();
    inline MemoryPool &operator=(MemoryPool &&) noexcept;

    inline void reset() noexcept;
    inline void *allocateBlock();
    inline void freeBlock(void *) noexcept;

private:
    const std::size_t blockAlignment_;
    const std::size_t blockSize_;
    const std::size_t firstChunkSize_;
    std::size_t chunkSize_;
    std::vector<void *> chunks_;
    void *lastFreeBlock_;

    inline void initialize() noexcept;
    inline void finalize() noexcept;
    inline void move(MemoryPool *) noexcept;

    void makeFreeBlocks();
};

}


/*
 * #include "memory_pool-inl.h"
 */


#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <utility>

#include "helper_macros.h"
#include "next_power_of_two.h"


namespace siren {

MemoryPool::MemoryPool(std::size_t blockAlignment, std::size_t blockSize
                       , std::size_t firstChunkLength) noexcept
  : blockAlignment_(std::max(NextPowerOfTwo(blockAlignment), alignof(void *))),
    blockSize_(SIREN_ALIGN(std::max(blockSize, sizeof(void *)), blockAlignment_)),
    firstChunkSize_(std::max(NextPowerOfTwo(firstChunkLength), std::size_t(1)) * blockSize_)
{
    assert(blockAlignment_ <= alignof(std::max_align_t));
    initialize();
}


MemoryPool::MemoryPool(MemoryPool &&other) noexcept
  : blockAlignment_(other.blockAlignment_),
    blockSize_(other.blockSize_),
    firstChunkSize_(other.firstChunkSize_),
    chunks_(std::move(other.chunks_))
{
    other.move(this);
}


MemoryPool::~MemoryPool()
{
    finalize();
}


MemoryPool &
MemoryPool::operator=(MemoryPool &&other) noexcept
{
    if (&other != this) {
        finalize();
        assert(blockAlignment_ == other.blockAlignment_);
        assert(blockSize_ == other.blockSize_);
        assert(firstChunkSize_ == other.firstChunkSize_);
        chunks_ = std::move(other.chunks_);
        other.move(this);
    }

    return *this;
}


void
MemoryPool::initialize() noexcept
{
    chunkSize_ = firstChunkSize_;
    lastFreeBlock_ = nullptr;
}


void
MemoryPool::finalize() noexcept
{
    for (void *chunk : chunks_) {
        std::free(chunk);
    }
}


void
MemoryPool::move(MemoryPool *other) noexcept
{
    other->chunkSize_ = chunkSize_;
    other->lastFreeBlock_ = lastFreeBlock_;
    initialize();
}


void
MemoryPool::reset() noexcept
{
    finalize();
    chunks_.clear();
    initialize();
}


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

}
