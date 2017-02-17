#include "memory_pool.h"

#include <cerrno>
#include <cstdlib>
#include <algorithm>
#include <system_error>
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
    nextChunkSize_ = firstChunkSize_;
    lastBlock_ = nullptr;
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
    other->nextChunkSize_ = nextChunkSize_;
    other->lastBlock_ = lastBlock_;
    initialize();
}


void
MemoryPool::reset() noexcept
{
    finalize();
    chunks_.clear();
    initialize();
}


void
MemoryPool::makeBlocks()
{
    std::size_t chunkSize = nextChunkSize_;
    void *chunk = std::malloc(chunkSize);

    if (chunk == nullptr) {
        throw std::system_error(errno, std::system_category(), "malloc() failed");
    }

    chunks_.push_back(chunk);
    nextChunkSize_ = 2 * chunkSize;
    void *block = static_cast<char *>(chunk) + chunkSize - blockSize_;

    do {
        *static_cast<void **>(block) = lastBlock_;
        lastBlock_ = block;
        block = static_cast<char *>(block) - blockSize_;
    } while (block >= chunk);
}

} // namespace siren
