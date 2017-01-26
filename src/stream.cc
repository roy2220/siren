#include "stream.h"

#include <cstring>
#include <utility>


namespace siren {

Stream::Stream() noexcept
{
    initialize();
}


Stream::Stream(Stream &&other) noexcept
  : buffer_(std::move(other.buffer_))
{
    other.move(this);
}


Stream &
Stream::operator=(Stream &&other) noexcept
{
    if (&other != this) {
        buffer_ = std::move(other.buffer_);
        other.move(this);
    }

    return *this;
}


void
Stream::initialize() noexcept
{
    readerIndex_ = 0;
    writerIndex_ = 0;
}


void
Stream::move(Stream *other) noexcept
{
    other->readerIndex_ = readerIndex_;
    other->writerIndex_ = writerIndex_;
    initialize();
}


void
Stream::reset() noexcept
{
    buffer_.reset();
    initialize();
}


void
Stream::dropData(std::size_t dataSize) noexcept
{
    assert(readerIndex_ + dataSize <= writerIndex_);
    readerIndex_ += dataSize;

    if (readerIndex_ >= writerIndex_ - readerIndex_) {
        std::memcpy(buffer_, buffer_ + readerIndex_, writerIndex_ - readerIndex_);
        writerIndex_ -= readerIndex_;
        readerIndex_ = 0;
    }
}


void
Stream::reserveBuffer(std::size_t bufferSize)
{
    if (buffer_.getLength() < writerIndex_ + bufferSize) {
        buffer_.setLength(writerIndex_ + bufferSize);
    }
}


void
Stream::read(void *buffer, std::size_t bufferSize) noexcept
{
    assert(bufferSize <= getDataSize());
    std::memcpy(buffer, getData(), bufferSize);
    dropData(bufferSize);
}


void
Stream::write(const void *data, std::size_t dataSize)
{
    reserveBuffer(dataSize);
    std::memcpy(getBuffer(), data, dataSize);
    pickData(dataSize);
}

} // namespace siren
