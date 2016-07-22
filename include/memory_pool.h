#pragma once


#include <cstddef>
#include <vector>


namespace siren {

class MemoryPool final
{
public:
    inline explicit MemoryPool(std::size_t, std::size_t, std::size_t) noexcept;
    inline MemoryPool(MemoryPool &&) noexcept;
    inline ~MemoryPool();
    inline MemoryPool &operator=(MemoryPool &&) noexcept;

    inline void reset() noexcept;
    inline void *allocateBlock();
    inline void freeBlock(void *) noexcept;

private:
    const std::size_t blockAlignment_;
    const std::size_t blockSize_;
    std::size_t firstChunkSize_;
    std::size_t chunkSize_;
    std::vector<void *> chunks_;
    void *lastFreeBlock_;
#ifndef NDEBUG
    std::size_t allocatedBlockCount_;
#endif

    inline void finalize() noexcept;
    inline void initialize() noexcept;

    void addBlocks();

    MemoryPool(const MemoryPool &) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;
};

}


/*
 * #include "memory_pool-inl.h"
 */


#include <cassert>
#include <cstdlib>
#include <utility>

#include "helper_macros.h"
#include "next_power_of_two.h"


namespace siren {

MemoryPool::MemoryPool(std::size_t blockAlignment, std::size_t blockSize
                       , std::size_t firstChunkLength) noexcept
  : blockAlignment_(blockAlignment < alignof(void *) ? alignof(void *)
                                                     : NextPowerOfTwo(blockAlignment)),
    blockSize_(blockSize < sizeof(void *) ? sizeof(void *)
                                          : SIREN_ALIGN(blockSize, blockAlignment_)),
    firstChunkSize_(NextPowerOfTwo((firstChunkLength < 1 ? 1 : firstChunkLength) * blockSize_)),
    chunkSize_(firstChunkSize_),
    lastFreeBlock_(nullptr)
#ifndef NDEBUG
        ,
    allocatedBlockCount_(0)
#endif

{
    assert(blockAlignment_ <= alignof(std::max_align_t));
}


MemoryPool::MemoryPool(MemoryPool &&other) noexcept
  : blockAlignment_(other.blockAlignment_),
    blockSize_(other.blockSize_),
    firstChunkSize_(other.firstChunkSize_),
    chunkSize_(other.chunkSize_),
    chunks_(std::move(other.chunks_)),
    lastFreeBlock_(other.lastFreeBlock_)
#ifndef NDEBUG
        ,
    allocatedBlockCount_(other.allocatedBlockCount_)
#endif
{
    other.initialize();
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
        firstChunkSize_ = other.firstChunkSize_;
        chunkSize_ = other.chunkSize_;
        chunks_ = std::move(other.chunks_);
        lastFreeBlock_ = other.lastFreeBlock_;
#ifndef NDEBUG
        allocatedBlockCount_ = other.allocatedBlockCount_;
#endif
        other.initialize();
    }

    return *this;
}


void
MemoryPool::reset() noexcept
{
    finalize();
    chunks_.clear();
    initialize();
}


void
MemoryPool::finalize() noexcept
{
    assert(allocatedBlockCount_ == 0);

    for (void *chunk : chunks_) {
        std::free(chunk);
    }
}


void
MemoryPool::initialize() noexcept
{
    chunkSize_ = firstChunkSize_;
    lastFreeBlock_ = nullptr;
#ifndef NDEBUG
    allocatedBlockCount_ = 0;
#endif
}


void *
MemoryPool::allocateBlock()
{
    if (lastFreeBlock_ == nullptr) {
        addBlocks();
    }

    void *block = lastFreeBlock_;
    lastFreeBlock_ = *static_cast<void **>(block);
#ifndef NDEBUG
    ++allocatedBlockCount_;
#endif
    return block;
}


void
MemoryPool::freeBlock(void *block) noexcept
{
    if (block != nullptr) {
        assert(allocatedBlockCount_ >= 1);
        *static_cast<void **>(block) = lastFreeBlock_;
        lastFreeBlock_ = block;
#ifndef NDEBUG
        --allocatedBlockCount_;
#endif
    }
}

}
