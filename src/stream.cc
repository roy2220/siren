#include "stream.h"

#include <cstring>
#include <utility>


namespace siren {

Stream::Stream() noexcept
{
    initialize();
}


Stream::Stream(Stream &&other) noexcept
  : base_(std::move(other.base_))
{
    other.move(this);
}


Stream &
Stream::operator=(Stream &&other) noexcept
{
    if (&other != this) {
        base_ = std::move(other.base_);
        other.move(this);
    }

    return *this;
}


void
Stream::initialize() noexcept
{
    dataOffset_ = 0;
    bufferOffset_ = 0;
}


void
Stream::move(Stream *other) noexcept
{
    other->dataOffset_ = dataOffset_;
    other->bufferOffset_ = bufferOffset_;
    initialize();
}


void
Stream::reset() noexcept
{
    base_.reset();
    initialize();
}


void
Stream::discardData(std::size_t dataSize) noexcept
{
    SIREN_ASSERT(dataOffset_ + dataSize <= bufferOffset_);
    dataOffset_ += dataSize;

    if (dataOffset_ >= bufferOffset_ - dataOffset_) {
        std::memcpy(base_, base_ + dataOffset_, bufferOffset_ - dataOffset_);
        bufferOffset_ -= dataOffset_;
        dataOffset_ = 0;
    }
}


void
Stream::reserveBuffer(std::size_t bufferSize)
{
    if (base_.getLength() < bufferOffset_ + bufferSize) {
        if (base_.getLength() < bufferOffset_ - dataOffset_ + bufferSize) {
            base_.setLength(bufferOffset_ + bufferSize);
        } else {
            std::memmove(base_, base_ + dataOffset_, bufferOffset_ - dataOffset_);
            bufferOffset_ -= dataOffset_;
            dataOffset_ = 0;
        }
    }
}


void
Stream::read(void *buffer, std::size_t bufferSize) noexcept
{
    SIREN_ASSERT(bufferSize <= getDataSize());
    std::memcpy(buffer, getData(), bufferSize);
    discardData(bufferSize);
}


void
Stream::write(const void *data, std::size_t dataSize)
{
    reserveBuffer(dataSize);
    std::memcpy(getBuffer(), data, dataSize);
    commitBuffer(dataSize);
}


EndOfStream::EndOfStream() noexcept
{
}


const char *
EndOfStream::what() const noexcept
{
    return "End of stream";
}

} // namespace siren
