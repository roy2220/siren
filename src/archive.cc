#include "archive.h"

#include <cstring>


namespace siren {

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

}
