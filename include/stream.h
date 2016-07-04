#pragma once


#include <cstddef>

#include "buffer.h"


namespace siren {

class Stream final
{
public:
    inline explicit Stream();

    inline const void *getData(std::ptrdiff_t = 0) const;
    inline void *getData(std::ptrdiff_t = 0);
    inline std::ptrdiff_t getDataSize() const;
    inline void pickData(std::ptrdiff_t);
    inline void dropData(std::ptrdiff_t);
    inline void *getBuffer(std::ptrdiff_t = 0);
    inline std::ptrdiff_t getBufferSize() const;
    inline void reserveBuffer(std::ptrdiff_t);

private:
    Buffer<char> buffer_;
    std::ptrdiff_t readerIndex_;
    std::ptrdiff_t writerIndex_;

    Stream(const Stream &) = delete;
    Stream &operator=(const Stream &) = delete;
};

}


/*
 * #include "stream-inl.h"
 */


#include <cassert>
#include <cstring>

#include "next_power_of_two.h"


namespace siren {

Stream::Stream()
  : readerIndex_(0),
    writerIndex_(0)
{
}


const void *
Stream::getData(std::ptrdiff_t dataOffset) const
{
    return buffer_.get() + readerIndex_ + dataOffset;
}


void *
Stream::getData(std::ptrdiff_t dataOffset)
{
    return buffer_.get() + readerIndex_ + dataOffset;
}


std::ptrdiff_t
Stream::getDataSize() const
{
    return writerIndex_ - readerIndex_;
}


void
Stream::pickData(std::ptrdiff_t dataSize)
{
    assert(dataSize >= 0);
    assert(writerIndex_ + dataSize <= buffer_.getLength());
    writerIndex_ += dataSize;
}


void
Stream::dropData(std::ptrdiff_t dataSize)
{
    assert(dataSize >= 0);
    assert(readerIndex_ + dataSize <= writerIndex_);
    readerIndex_ += dataSize;

    if (readerIndex_ >= writerIndex_ - readerIndex_) {
        std::memcpy(buffer_.get(), buffer_.get() + readerIndex_, writerIndex_ - readerIndex_);
        writerIndex_ -= readerIndex_;
        readerIndex_ = 0;
    }
}


void *
Stream::getBuffer(std::ptrdiff_t bufferOffset)
{
    return buffer_.get() + writerIndex_ + bufferOffset;
}


std::ptrdiff_t
Stream::getBufferSize() const
{
    return buffer_.getLength() - writerIndex_;
}


void
Stream::reserveBuffer(std::ptrdiff_t bufferSize)
{
    assert(bufferSize >= 0);

    if (buffer_.getLength() < writerIndex_ + bufferSize) {
        buffer_.setLength(writerIndex_ + bufferSize);
    }
}

}
