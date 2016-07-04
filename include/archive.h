#pragma once


#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <type_traits>
#include <vector>


#define SIREN_SERDES(...)                               \
    void serialize(::siren::Archive *archive) const {   \
        (::siren::Serializer_(archive)), __VA_ARGS__;   \
    }                                                   \
                                                        \
    void deserialize(::siren::Archive *archive) {       \
        (::siren::Deserializer_(archive)), __VA_ARGS__; \
    }


namespace siren {

class ArchiveEndOfStream;
class Stream;


class Archive final
{
public:
    typedef ArchiveEndOfStream EndOfStream;

    inline explicit Archive(Stream *);

    template <class T>
    inline std::enable_if_t<std::is_unsigned<T>::value, Archive &> operator<<(T);

    template <class T>
    inline std::enable_if_t<std::is_unsigned<T>::value, Archive &> operator>>(T &);

    template <class T>
    inline std::enable_if_t<std::is_signed<T>::value, Archive &> operator<<(T);

    template <class T>
    inline std::enable_if_t<std::is_signed<T>::value, Archive &> operator>>(T &);

    template <class T>
    inline std::enable_if_t<std::is_enum<T>::value, Archive &> operator<<(T);

    template <class T>
    inline std::enable_if_t<std::is_enum<T>::value, Archive &> operator>>(T &);

    template <class T, std::size_t N>
    inline std::enable_if_t<sizeof(T) == sizeof(char) && alignof(T) == alignof(char)
                            , Archive &> operator<<(const T (&)[N]);

    template <class T, std::size_t N>
    inline std::enable_if_t<sizeof(T) == sizeof(char) && alignof(T) == alignof(char)
                            , Archive &> operator>>(T (&)[N]);

    template <class T, std::size_t N>
    inline std::enable_if_t<sizeof(T) != sizeof(char) || alignof(T) != alignof(char)
                            , Archive &> operator<<(const T (&)[N]);

    template <class T, std::size_t N>
    inline std::enable_if_t<sizeof(T) != sizeof(char) || alignof(T) != alignof(char)
                            , Archive &> operator>>(T (&)[N]);

    template <class T>
    inline std::enable_if_t<sizeof(T) == sizeof(char) && alignof(T) == alignof(char)
                            , Archive &> operator<<(const std::vector<T> &);

    template <class T>
    inline std::enable_if_t<sizeof(T) == sizeof(char) && alignof(T) == alignof(char)
                            , Archive &> operator>>(std::vector<T> &);

    template <class T>
    inline std::enable_if_t<sizeof(T) != sizeof(char) || alignof(T) != alignof(char)
                            , Archive &> operator<<(const std::vector<T> &);

    template <class T>
    inline std::enable_if_t<sizeof(T) != sizeof(char) || alignof(T) != alignof(char)
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
    inline std::enable_if_t<std::is_unsigned<T>::value, void> serializeInteger(T);

    template <class T>
    inline std::enable_if_t<std::is_unsigned<T>::value, void> deserializeInteger(T *);

    void serializeVariableLengthInteger(std::uintmax_t);
    void deserializeVariableLengthInteger(std::uintmax_t *);
    void serializeBytes(const void *, std::size_t);
    void deserializeBytes(void *, std::size_t);

    Archive(const Archive &) = delete;
    Archive &operator=(const Archive &) = delete;
};


class ArchiveEndOfStream final
  : public std::exception
{
public:
    inline const char *what() const noexcept override;

    ArchiveEndOfStream(ArchiveEndOfStream &&) = default;
    ArchiveEndOfStream &operator=(ArchiveEndOfStream &&) = default;

private:
    inline explicit ArchiveEndOfStream();

    ArchiveEndOfStream(const ArchiveEndOfStream &) = delete;
    ArchiveEndOfStream &operator=(const ArchiveEndOfStream &) = delete;

    friend Archive;
};


class Serializer_ final
{
public:
    inline explicit Serializer_(Archive *);

    template <class T>
    inline const Serializer_ &operator,(const T &) const;

private:
    Archive *const archive_;

    Serializer_(const Serializer_ &) = delete;
    Serializer_ &operator=(const Serializer_ &) = delete;
};


class Deserializer_ final
{
public:
    inline explicit Deserializer_(Archive *);

    template <class T>
    inline const Deserializer_ &operator,(T &) const;

private:
    Archive *const archive_;

    Deserializer_(const Deserializer_ &) = delete;
    Deserializer_ &operator=(const Deserializer_ &) = delete;
};

}


/*
 * #include "archive-inl.h"
 */


#include <cassert>
#include <limits>

#include "stream.h"
#include "unsigned_to_signed.h"


namespace siren {

Archive::Archive(Stream *stream)
  : stream_(stream),
    writtenByteCount_(0),
    readByteCount_(0)
{
    assert(stream_ != nullptr);
}


template <class T>
std::enable_if_t<std::is_unsigned<T>::value, Archive &>
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
Archive::operator<<(T integer)
{
    typedef std::make_unsigned_t<T> U;

    serializeInteger<U>(integer);
    return *this;
}


template <class T>
std::enable_if_t<std::is_signed<T>::value, Archive &>
Archive::operator>>(T &integer)
{
    typedef std::make_unsigned_t<T> U;

    U temp;
    integer = UnsignedToSigned((deserializeInteger(&temp), temp));
    return *this;
}


template <class T>
std::enable_if_t<std::is_enum<T>::value, Archive &>
Archive::operator<<(T enumerator)
{
    typedef std::underlying_type_t<T> U;

    operator<<(static_cast<U>(enumerator));
    return *this;
}


template <class T>
std::enable_if_t<std::is_enum<T>::value, Archive &>
Archive::operator>>(T &enumerator)
{
    typedef std::underlying_type_t<T> U;

    U temp;
    enumerator = static_cast<T>(operator>>(temp), temp);
    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<sizeof(T) == sizeof(char) && alignof(T) == alignof(char), Archive &>
Archive::operator<<(const T (&array)[N])
{
    serializeBytes(array, N);
    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<sizeof(T) == sizeof(char) && alignof(T) == alignof(char), Archive &>
Archive::operator>>(T (&array)[N])
{
    deserializeBytes(array, N);
    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<sizeof(T) != sizeof(char) || alignof(T) != alignof(char), Archive &>
Archive::operator<<(const T (&array)[N])
{
    for (const T &x : array) {
        operator<<(x);
    }

    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<sizeof(T) != sizeof(char) || alignof(T) != alignof(char), Archive &>
Archive::operator>>(T (&array)[N])
{
    for (T &x : array) {
        operator>>(x);
    }

    return *this;
}


template <class T>
std::enable_if_t<sizeof(T) == sizeof(char) && alignof(T) == alignof(char), Archive &>
Archive::operator<<(const std::vector<T> &vector)
{
    serializeVariableLengthInteger(vector.size());
    serializeBytes(&vector.front(), vector.size());
    return *this;
}


template <class T>
std::enable_if_t<sizeof(T) == sizeof(char) && alignof(T) == alignof(char), Archive &>
Archive::operator>>(std::vector<T> &vector)
{
    std::size_t temp;
    vector.resize((deserializeVariableLengthInteger(&temp), temp));
    deserializeBytes(&vector.front(), vector.size());
    return *this;
}


template <class T>
std::enable_if_t<sizeof(T) != sizeof(char) || alignof(T) != alignof(char), Archive &>
Archive::operator<<(const std::vector<T> &vector)
{
    serializeVariableLengthInteger(vector.size());

    for (const T &x : vector) {
        operator<<(x);
    }

    return *this;
}


template <class T>
std::enable_if_t<sizeof(T) != sizeof(char) || alignof(T) != alignof(char), Archive &>
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
std::enable_if_t<std::is_unsigned<T>::value, void>
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
std::enable_if_t<std::is_unsigned<T>::value, void>
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


ArchiveEndOfStream::ArchiveEndOfStream()
{
}


const char *
ArchiveEndOfStream::what() const noexcept
{
    return "Archive: End of stream";
}


Serializer_::Serializer_(Archive *archive)
  : archive_(archive)
{
}


template <class T>
const Serializer_ &
Serializer_::operator,(const T &x) const
{
    *archive_ << x;
    return *this;
}


Deserializer_::Deserializer_(Archive *archive)
  : archive_(archive)
{
}


template <class T>
const Deserializer_ &
Deserializer_::operator,(T &x) const
{
    *archive_ >> x;
    return *this;
}

}
