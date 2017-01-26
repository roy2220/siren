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
    inline void pickData(std::size_t) noexcept;
    inline void *getBuffer(std::size_t = 0) noexcept;
    inline std::size_t getBufferSize() const noexcept;

    explicit Stream() noexcept;
    Stream(Stream &&) noexcept;
    Stream &operator=(Stream &&) noexcept;

    void reset() noexcept;
    void dropData(std::size_t) noexcept;
    void reserveBuffer(std::size_t);
    void read(void *, std::size_t) noexcept;
    void write(const void *, std::size_t);

private:
    Buffer<char> buffer_;
    std::size_t readerIndex_;
    std::size_t writerIndex_;

    void initialize() noexcept;
    void move(Stream *) noexcept;
};

} // namespace siren


/*
 * #include "stream-inl.h"
 */


#include <cassert>


namespace siren {

const void *
Stream::getData(std::size_t dataOffset) const noexcept
{
    return buffer_ + readerIndex_ + dataOffset;
}


void *
Stream::getData(std::size_t dataOffset) noexcept
{
    return buffer_ + readerIndex_ + dataOffset;
}


std::size_t
Stream::getDataSize() const noexcept
{
    return writerIndex_ - readerIndex_;
}


void
Stream::pickData(std::size_t dataSize) noexcept
{
    assert(writerIndex_ + dataSize <= buffer_.getLength());
    writerIndex_ += dataSize;
}


void *
Stream::getBuffer(std::size_t bufferOffset) noexcept
{
    return buffer_ + writerIndex_ + bufferOffset;
}


std::size_t
Stream::getBufferSize() const noexcept
{
    return buffer_.getLength() - writerIndex_;
}

} // namespace siren
