#pragma once


#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <type_traits>
#include <vector>


namespace siren {

class ArchiveEndOfStream;
class Stream;


class Archive final
{
    Archive(const Archive &) = delete;
    void operator=(const Archive &) = delete;

public:
    typedef ArchiveEndOfStream EndOfStream;

    inline explicit Archive(Stream *);

    template <class T>
    inline std::enable_if_t<std::is_unsigned<T>::value || std::is_signed<T>::value
                            , Archive &> operator<<(T);

    template <class T>
    inline std::enable_if_t<std::is_unsigned<T>::value, Archive &> operator>>(T &);

    template <class T>
    inline std::enable_if_t<std::is_signed<T>::value, Archive &> operator>>(T &);

    template <class T>
    inline std::enable_if_t<std::is_enum<T>::value, Archive &> operator<<(T);

    template <class T>
    inline std::enable_if_t<std::is_enum<T>::value, Archive &> operator>>(T &);

    template <class T, std::size_t N>
    inline std::enable_if_t<std::is_same<T, unsigned char>::value
                            || std::is_same<T, signed char>::value
                            , Archive &> operator<<(const T (&)[N]);

    template <class T, std::size_t N>
    inline std::enable_if_t<std::is_same<T, unsigned char>::value
                            || std::is_same<T, signed char>::value
                            , Archive &> operator>>(T (&)[N]);

    template <class T, std::size_t N>
    inline std::enable_if_t<!std::is_same<T, unsigned char>::value
                            && !std::is_same<T, signed char>::value
                            , Archive &> operator<<(const T (&)[N]);

    template <class T, std::size_t N>
    inline std::enable_if_t<!std::is_same<T, unsigned char>::value
                            && !std::is_same<T, signed char>::value
                            , Archive &> operator>>(T (&)[N]);

    template <class T>
    inline std::enable_if_t<std::is_same<T, unsigned char>::value
                            || std::is_same<T, signed char>::value
                            , Archive &> operator<<(const std::vector<T> &);

    template <class T>
    inline std::enable_if_t<std::is_same<T, unsigned char>::value
                            || std::is_same<T, signed char>::value
                            , Archive &> operator>>(std::vector<T> &);

    template <class T>
    inline std::enable_if_t<!std::is_same<T, unsigned char>::value
                            && !std::is_same<T, signed char>::value
                            , Archive &> operator<<(const std::vector<T> &);

    template <class T>
    inline std::enable_if_t<!std::is_same<T, unsigned char>::value
                            && !std::is_same<T, signed char>::value
                            , Archive &> operator>>(std::vector<T> &);

    template <class T>
    inline std::enable_if_t<std::is_class<T>::value, Archive &> operator<<(const T &);

    template <class T>
    inline std::enable_if_t<std::is_class<T>::value, Archive &> operator>>(T &);

    inline Archive &operator<<(bool);
    inline Archive &operator>>(bool &);
    inline Archive &operator<<(float);
    inline Archive &operator>>(float &);
    inline Archive &operator<<(double);
    inline Archive &operator>>(double &);
    inline Archive &operator<<(const std::string &);
    inline Archive &operator>>(std::string &);

    inline void flush();

private:
    Stream *const stream_;
    std::size_t writtenByteCount_;
    std::size_t readByteCount_;

    template <class T>
    inline void serializeInteger(T);

    template <class T>
    inline void deserializeInteger(T *);

    void serializeVariableLengthInteger(std::uintmax_t);
    void deserializeVariableLengthInteger(std::uintmax_t *);
    void serializeBytes(const void *, std::size_t);
    void deserializeBytes(void *, std::size_t);
};


class ArchiveEndOfStream final
  : public std::exception
{
    ArchiveEndOfStream(const ArchiveEndOfStream &) = delete;
    void operator=(const ArchiveEndOfStream &) = delete;

public:
    inline ArchiveEndOfStream(ArchiveEndOfStream &&);

    inline const char *what() const noexcept override;

private:
    inline explicit ArchiveEndOfStream() = default;

    friend Archive;
};

}


/*
 * #include "archive-inl.h"
 */


#include <limits>

#include "stream.h"
#include "unsigned_to_signed.h"


namespace siren {

Archive::Archive(Stream *stream)
  : stream_(stream),
    writtenByteCount_(0),
    readByteCount_(0)
{
}


template <class T>
std::enable_if_t<std::is_unsigned<T>::value || std::is_signed<T>::value, Archive &>
Archive::operator<<(T integer)
{
    serializeInteger(integer);
    return *this;
}


template <class T>
std::enable_if_t<std::is_unsigned<T>::value, Archive &>
Archive::operator>>(T &integer)
{
    deserializeInteger(&integer);
    return *this;
}


template <class T>
std::enable_if_t<std::is_signed<T>::value, Archive &>
Archive::operator>>(T &integer)
{
    using U = std::make_unsigned_t<T>;

    U temp;
    integer = UnsignedToSigned((deserializeInteger(&temp), temp));
    return *this;
}


template <class T>
std::enable_if_t<std::is_enum<T>::value, Archive &>
Archive::operator<<(T enumerator)
{
    using U = std::underlying_type_t<T>;

    operator<<(static_cast<U>(enumerator));
    return *this;
}


template <class T>
std::enable_if_t<std::is_enum<T>::value, Archive &>
Archive::operator>>(T &enumerator)
{
    using U = std::underlying_type_t<T>;

    U temp;
    enumerator = static_cast<T>(operator>>(temp), temp);
    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<std::is_same<T, unsigned char>::value || std::is_same<T, signed char>::value
                 , Archive &>
Archive::operator<<(const T (&array)[N])
{
    serializeBytes(array, N);
    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<std::is_same<T, unsigned char>::value || std::is_same<T, signed char>::value
                 , Archive &>
Archive::operator>>(T (&array)[N])
{
    deserializeBytes(array, N);
    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<!std::is_same<T, unsigned char>::value && !std::is_same<T, signed char>::value
                 , Archive &>
Archive::operator<<(const T (&array)[N])
{
    for (const T &x : array) {
        operator<<(x);
    }

    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<!std::is_same<T, unsigned char>::value && !std::is_same<T, signed char>::value
                 , Archive &>
Archive::operator>>(T (&array)[N])
{
    for (T &x : array) {
        operator>>(x);
    }

    return *this;
}


template <class T>
std::enable_if_t<std::is_same<T, unsigned char>::value || std::is_same<T, signed char>::value
                 , Archive &>
Archive::operator<<(const std::vector<T> &vector)
{
    serializeVariableLengthInteger(vector.size());
    serializeBytes(&vector.front(), vector.size());
    return *this;
}


template <class T>
std::enable_if_t<std::is_same<T, unsigned char>::value || std::is_same<T, signed char>::value
                 , Archive &>
Archive::operator>>(std::vector<T> &vector)
{
    std::size_t temp;
    vector.resize((deserializeVariableLengthInteger(&temp), temp));
    deserializeBytes(&vector.front(), vector.size());
    return *this;
}


template <class T>
std::enable_if_t<!std::is_same<T, unsigned char>::value && !std::is_same<T, signed char>::value
                 , Archive &>
Archive::operator<<(const std::vector<T> &vector)
{
    serializeVariableLengthInteger(vector.size());

    for (const T &x : vector) {
        operator<<(x);
    }

    return *this;
}


template <class T>
std::enable_if_t<!std::is_same<T, unsigned char>::value && !std::is_same<T, signed char>::value
                 , Archive &>
Archive::operator>>(std::vector<T> &vector)
{
    std::size_t temp;
    vector.resize((deserializeVariableLengthInteger(&temp), temp));

    for (T &x : vector) {
        operator>>(x);
    }

    return *this;
}


template <class T>
std::enable_if_t<std::is_class<T>::value, Archive &>
Archive::operator<<(const T &object)
{
    object.serialize(this);
    return *this;
}


template <class T>
std::enable_if_t<std::is_class<T>::value, Archive &>
Archive::operator>>(T &object)
{
    object.deserialize(this);
    return *this;
}


Archive &
Archive::operator<<(bool boolean)
{
    operator<<(static_cast<unsigned char>(boolean));
    return *this;
}


Archive &
Archive::operator>>(bool &boolean)
{
    unsigned char temp;
    boolean = (operator>>(temp), temp);
    return *this;
}


Archive &
Archive::operator<<(float floatingPoint)
{
    static_assert(sizeof(float) == sizeof(std::uint32_t)
                  && alignof(float) == alignof(std::uint32_t), "");
    operator<<(reinterpret_cast<std::uint32_t &>(floatingPoint));
    return *this;
}


Archive &
Archive::operator>>(float &floatingPoint)
{
    static_assert(sizeof(float) == sizeof(std::uint32_t)
                  && alignof(float) == alignof(std::uint32_t), "");
    operator>>(reinterpret_cast<std::uint32_t &>(floatingPoint));
    return *this;
}


Archive &
Archive::operator<<(double floatingPoint)
{
    static_assert(sizeof(double) == sizeof(std::uint64_t)
                  && alignof(double) == alignof(std::uint64_t), "");
    operator<<(reinterpret_cast<std::uint64_t &>(floatingPoint));
    return *this;
}


Archive &
Archive::operator>>(double &floatingPoint)
{
    static_assert(sizeof(double) == sizeof(std::uint64_t)
                  && alignof(double) == alignof(std::uint64_t), "");
    operator>>(reinterpret_cast<std::uint64_t &>(floatingPoint));
    return *this;
}


Archive &
Archive::operator<<(const std::string &string)
{
    serializeVariableLengthInteger(string.size());
    serializeBytes(&string.front(), string.size());
    return *this;
}


Archive &
Archive::operator>>(std::string &string)
{
    std::size_t temp;
    string.resize((deserializeVariableLengthInteger(&temp), temp));
    deserializeBytes(&string.front(), string.size());
    return *this;
}


void
Archive::flush()
{
    stream_->pickData(writtenByteCount_);
    writtenByteCount_ = 0;
    stream_->dropData(readByteCount_);
    readByteCount_ = 0;
}


template <class T>
void
Archive::serializeInteger(T integer)
{
    constexpr int k1 = std::numeric_limits<T>::digits;
    constexpr int k2 = std::numeric_limits<unsigned char>::digits;

    std::size_t bufferSize = stream_->getBufferSize() - writtenByteCount_;

    if (bufferSize < sizeof(T)) {
        stream_->growBuffer(sizeof(T) - bufferSize);
    }

    auto buffer = static_cast<unsigned char *>(stream_->getBuffer(writtenByteCount_));
    *buffer = integer;

    for (int n = k1 - k2; n >= 1; n -= k2) {
        *++buffer = (integer >>= k2);
    }

    writtenByteCount_ += sizeof(T);
}


template <class T>
void
Archive::deserializeInteger(T *integer)
{
    constexpr int k1 = std::numeric_limits<T>::digits;
    constexpr int k2 = std::numeric_limits<unsigned char>::digits;

    std::size_t dataSize = stream_->getDataSize() - readByteCount_;

    if (dataSize < sizeof(T)) {
        throw EndOfStream();
    }

    auto data = static_cast<unsigned char *>(stream_->getData(readByteCount_));
    *integer = *data;

    for (int n = k2; n < k1; n += k2) {
        *integer |= static_cast<T>(*++data) << n;
    }

    readByteCount_ += sizeof(T);
}


ArchiveEndOfStream::ArchiveEndOfStream(ArchiveEndOfStream &&other)
{
    static_cast<void>(other);
}


const char *
ArchiveEndOfStream::what() const noexcept
{
    return "end of stream";
}

}
