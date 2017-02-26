#include "archive.h"

#include <cstring>


namespace siren {

Archive::Archive(Stream *stream) noexcept
{
    SIREN_ASSERT(stream != nullptr);
    initialize(stream);
}


Archive::Archive(Archive &&other) noexcept
{
    other.move(this);
}


Archive::~Archive()
{
    finalize();
}


Archive &
Archive::operator=(Archive &&other) noexcept
{
    if (&other != this) {
        finalize();
        other.move(this);
    }

    return *this;
}


void
Archive::initialize(Stream *stream) noexcept
{
    stream_ = stream;
    writtenByteCount_ = 0;
    readByteCount_ = 0;
}


void
Archive::finalize() noexcept
{
    if (isValid() && !std::uncaught_exception()) {
        stream_->pickData(writtenByteCount_);
        stream_->dropData(readByteCount_);
    }
}


void
Archive::move(Archive *other) noexcept
{
    other->stream_ = stream_;
    stream_ = nullptr;
    other->writtenByteCount_ = writtenByteCount_;
    other->readByteCount_ = readByteCount_;
}


void
Archive::serializeVariableLengthInteger(std::uintmax_t integer)
{
    constexpr unsigned int k1 = std::numeric_limits<std::uintmax_t>::digits;
    constexpr unsigned int k2 = std::numeric_limits<unsigned char>::digits - 1;
    constexpr unsigned char k3 = std::numeric_limits<unsigned char>::max() >> 1;

    serializeInteger<unsigned char>(integer & k3);

    for (unsigned int n = k1 - k2; UnsignedToSigned(n) >= 1; n -= k2) {
        if ((((integer >> k2) ^ (integer >> (k2 - 1))) & ((UINTMAX_C(1) << n) - 1)) == 0) {
            auto buffer = static_cast<unsigned char *>(stream_->getBuffer(writtenByteCount_ - 1));
            *buffer |= k3 + 1;
            return;
        } else {
            serializeInteger<unsigned char>((integer >>= k2) & k3);
        }
    }
}


void
Archive::deserializeVariableLengthInteger(std::uintmax_t *integer)
{
    constexpr unsigned int k1 = std::numeric_limits<std::uintmax_t>::digits;
    constexpr unsigned int k2 = std::numeric_limits<unsigned char>::digits - 1;
    constexpr unsigned char k3 = std::numeric_limits<unsigned char>::max() >> 1;

    unsigned char temp;
    *integer = (deserializeInteger(&temp), temp & k3);

    for (unsigned int n = k2; n < k1; n += k2) {
        if ((temp & (k3 + 1)) == k3 + 1) {
            *integer |= -(*integer & (UINTMAX_C(1) << (n - 1)));
            return;
        } else {
            *integer |= static_cast<std::uintmax_t>(deserializeInteger(&temp), temp & k3) << n;
        }
    }
}


void
Archive::serializeBytes(const void *bytes, std::size_t numberOfBytes)
{
    stream_->reserveBuffer(writtenByteCount_ + numberOfBytes);
    void *buffer = stream_->getBuffer(writtenByteCount_);
    std::memcpy(buffer, bytes, numberOfBytes);
    writtenByteCount_ += numberOfBytes;
}


void
Archive::deserializeBytes(void *bytes, std::size_t numberOfBytes)
{
    if (stream_->getDataSize() < readByteCount_ + numberOfBytes) {
        throw EndOfStream();
    }

    void *data = stream_->getData(readByteCount_);
    std::memcpy(bytes, data, numberOfBytes);
    readByteCount_ += numberOfBytes;
}


ArchiveEndOfStream::ArchiveEndOfStream() noexcept
{
}


const char *
ArchiveEndOfStream::what() const noexcept
{
    return "Siren: Archive: End of stream";
}

} // namespace siren
