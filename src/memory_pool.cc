#include "memory_pool.h"

#include <cerrno>
#include <system_error>


namespace siren {

void
MemoryPool::addBlocks()
{
    void *chunk = std::malloc(chunkSize_);

    if (chunk == nullptr) {
        throw std::system_error(errno, std::system_category(), "malloc() failed");
    }

    chunks_.push_back(chunk);
    void *firstFreeBlock = &lastFreeBlock_;
    void *block = static_cast<char *>(chunk) + chunkSize_ - blockSize_;

    do {
        *static_cast<void **>(firstFreeBlock) = block;
        firstFreeBlock = block;
        block = static_cast<char *>(block) - blockSize_;
    } while (block >= chunk);

    *static_cast<void **>(firstFreeBlock) = nullptr;
    chunkSize_ *= 2;
}

}
