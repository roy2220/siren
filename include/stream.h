#pragma once


#include <cstddef>

#include "buffer.h"


namespace siren {

class Stream final
{
public:
    inline const void *getData(std::size_t = 0) const noexcept;
    inline void *getData(std::size_t = 0) noexcept;
    inline std::size_t getDataSize() const noexcept;
    inline void commitData(std::size_t) noexcept;
    inline void *getBuffer(std::size_t = 0) noexcept;
    inline std::size_t getBufferSize() const noexcept;

    explicit Stream() noexcept;
    Stream(Stream &&) noexcept;
    Stream &operator=(Stream &&) noexcept;

    void reset() noexcept;
    void discardData(std::size_t) noexcept;
    void reserveBuffer(std::size_t);
    void read(void *, std::size_t) noexcept;
    void write(const void *, std::size_t);

private:
    Buffer<char> base_;
    std::size_t dataOffset_;
    std::size_t bufferOffset_;

    void initialize() noexcept;
    void move(Stream *) noexcept;
};

} // namespace siren


/*
 * #include "stream-inl.h"
 */


#include "assert.h"


namespace siren {

const void *
Stream::getData(std::size_t offset) const noexcept
{
    return base_ + dataOffset_ + offset;
}


void *
Stream::getData(std::size_t offset) noexcept
{
    return base_ + dataOffset_ + offset;
}


std::size_t
Stream::getDataSize() const noexcept
{
    return bufferOffset_ - dataOffset_;
}


void
Stream::commitData(std::size_t dataSize) noexcept
{
    SIREN_ASSERT(bufferOffset_ + dataSize <= base_.getLength());
    bufferOffset_ += dataSize;
}


void *
Stream::getBuffer(std::size_t offset) noexcept
{
    return base_ + bufferOffset_ + offset;
}


std::size_t
Stream::getBufferSize() const noexcept
{
    return base_.getLength() - bufferOffset_;
}

} // namespace siren
