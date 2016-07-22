#pragma once


#include <cstddef>

#include "buffer.h"


namespace siren {

class Stream final
{
public:
    inline explicit Stream();
    inline Stream(Stream &&);
    inline Stream &operator=(Stream &&);

    inline const void *getData(std::size_t = 0) const;
    inline void *getData(std::size_t = 0);
    inline std::size_t getDataSize() const;
    inline void pickData(std::size_t);
    inline void dropData(std::size_t);
    inline void *getBuffer(std::size_t = 0);
    inline std::size_t getBufferSize() const;
    inline void reserveBuffer(std::size_t);

private:
    Buffer<char> buffer_;
    std::size_t readerIndex_;
    std::size_t writerIndex_;

    inline void initialize();

    Stream(const Stream &) = delete;
    Stream &operator=(const Stream &) = delete;
};

}


/*
 * #include "stream-inl.h"
 */


#include <cassert>
#include <cstring>
#include <utility>

#include "next_power_of_two.h"


namespace siren {

Stream::Stream()
  : readerIndex_(0),
    writerIndex_(0)
{
}


Stream::Stream(Stream &&other)
  : buffer_(std::move(other.buffer_)),
    readerIndex_(other.readerIndex_),
    writerIndex_(other.writerIndex_)
{
    other.initialize();
}


Stream &
Stream::operator=(Stream &&other)
{
    if (&other != this) {
        buffer_ = std::move(other.buffer_);
        readerIndex_ = other.readerIndex_;
        writerIndex_ = other.writerIndex_;
        other.initialize();
    }

    return *this;
}


void
Stream::initialize()
{
    readerIndex_ = 0;
    writerIndex_ = 0;
}


const void *
Stream::getData(std::size_t dataOffset) const
{
    return buffer_ + readerIndex_ + dataOffset;
}


void *
Stream::getData(std::size_t dataOffset)
{
    return buffer_ + readerIndex_ + dataOffset;
}


std::size_t
Stream::getDataSize() const
{
    return writerIndex_ - readerIndex_;
}


void
Stream::pickData(std::size_t dataSize)
{
    assert(writerIndex_ + dataSize <= buffer_.getLength());
    writerIndex_ += dataSize;
}


void
Stream::dropData(std::size_t dataSize)
{
    assert(readerIndex_ + dataSize <= writerIndex_);
    readerIndex_ += dataSize;

    if (readerIndex_ >= writerIndex_ - readerIndex_) {
        std::memcpy(buffer_, buffer_ + readerIndex_, writerIndex_ - readerIndex_);
        writerIndex_ -= readerIndex_;
        readerIndex_ = 0;
    }
}


void *
Stream::getBuffer(std::size_t bufferOffset)
{
    return buffer_ + writerIndex_ + bufferOffset;
}


std::size_t
Stream::getBufferSize() const
{
    return buffer_.getLength() - writerIndex_;
}


void
Stream::reserveBuffer(std::size_t bufferSize)
{
    if (buffer_.getLength() < writerIndex_ + bufferSize) {
        buffer_.setLength(writerIndex_ + bufferSize);
    }
}

}
