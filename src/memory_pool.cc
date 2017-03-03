#include "memory_pool.h"

#include <cerrno>
#include <cstdlib>
#include <algorithm>
#include <system_error>
#include <utility>

#include "utility.h"


namespace siren {

MemoryPool::MemoryPool(std::size_t blockAlignment, std::size_t blockSize
                       , std::size_t minChunkLength) noexcept
  : blockAlignment_(std::max(NextPowerOfTwo(blockAlignment), std::size_t(1))),
    blockSize_(AlignSize(std::max(blockSize, sizeof(void *)), blockAlignment_)),
    minChunkSize_(NextPowerOfTwo(std::max(minChunkLength, std::size_t(1)) * blockSize_))
{
    SIREN_ASSERT(blockAlignment_ <= alignof(std::max_align_t));
    initialize();
}


MemoryPool::MemoryPool(MemoryPool &&other) noexcept
  : blockAlignment_(other.blockAlignment_),
    blockSize_(other.blockSize_),
    minChunkSize_(other.minChunkSize_),
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
        SIREN_ASSERT(blockAlignment_ == other.blockAlignment_);
        SIREN_ASSERT(blockSize_ == other.blockSize_);
        SIREN_ASSERT(minChunkSize_ == other.minChunkSize_);
        chunks_ = std::move(other.chunks_);
        other.move(this);
    }

    return *this;
}


void
MemoryPool::initialize() noexcept
{
    nextChunkSize_ = minChunkSize_;
    static char dummy[2];
    lastNewBlock_ = &dummy[0];
    firstNewBlock_ = &dummy[1];
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
    other->nextChunkSize_ = nextChunkSize_;
    other->lastNewBlock_ = lastNewBlock_;
    other->firstNewBlock_ = firstNewBlock_;
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
MemoryPool::makeBlock()
{
    void *block;

    if (firstNewBlock_ <= lastNewBlock_) {
        block = firstNewBlock_;
    } else {
        chunks_.reserve(chunks_.size() + 1);
        std::size_t chunkSize = nextChunkSize_;
        block = std::malloc(chunkSize);

        if (block == nullptr) {
            throw std::system_error(errno, std::system_category(), "malloc() failed");
        }

        chunks_.push_back(block);
        nextChunkSize_ = 2 * chunkSize;
        lastNewBlock_ = static_cast<char *>(block) + chunkSize - blockSize_;
    }

    firstNewBlock_ = static_cast<char *>(block) + blockSize_;
    return block;
}

} // namespace siren
