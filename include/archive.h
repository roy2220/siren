#pragma once


#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <type_traits>
#include <vector>


#define SIREN_SERDES(...)                                      \
    void serialize(::siren::Archive *archive) const {          \
        (::siren::detail::Serializer(archive)), __VA_ARGS__;   \
    }                                                          \
                                                               \
    void deserialize(::siren::Archive *archive) {              \
        (::siren::detail::Deserializer(archive)), __VA_ARGS__; \
    }

#define SIREN__LIKE_CHAR(T) \
    ((sizeof(T) == sizeof(char) && alignof(T) == alignof(char)) && std::is_pod<T>::value)


namespace siren {

class ArchiveEndOfStream;
class Stream;

template <class T, bool = std::is_integral<T>::value>
class VLI;


class Archive final
{
public:
    typedef ArchiveEndOfStream EndOfStream;

    inline Archive &operator<<(bool);
    inline Archive &operator>>(bool &);
    inline Archive &operator<<(float);
    inline Archive &operator>>(float &);
    inline Archive &operator<<(double);
    inline Archive &operator>>(double &);
    inline Archive &operator<<(const std::string &);
    inline Archive &operator>>(std::string &);

    template <class T>
    inline std::enable_if_t<std::is_integral<T>::value, Archive &> operator<<(T);

    template <class T>
    inline std::enable_if_t<std::is_integral<T>::value, Archive &> operator>>(T &);

    template <class T>
    inline Archive &operator<<(VLI<T>);

    template <class T>
    inline Archive &operator>>(VLI<T> &);

    template <class T>
    inline std::enable_if_t<std::is_enum<T>::value, Archive &> operator<<(T);

    template <class T>
    inline std::enable_if_t<std::is_enum<T>::value, Archive &> operator>>(T &);

    template <class T, std::size_t N>
    inline std::enable_if_t<SIREN__LIKE_CHAR(T), Archive &> operator<<(const T (&)[N]);

    template <class T, std::size_t N>
    inline std::enable_if_t<SIREN__LIKE_CHAR(T), Archive &> operator>>(T (&)[N]);

    template <class T>
    inline std::enable_if_t<SIREN__LIKE_CHAR(T), Archive &> operator<<(const std::vector<T> &);

    template <class T>
    inline std::enable_if_t<SIREN__LIKE_CHAR(T), Archive &> operator>>(std::vector<T> &);

    template <class T, std::size_t N>
    inline std::enable_if_t<!SIREN__LIKE_CHAR(T), Archive &> operator<<(const T (&)[N]);

    template <class T, std::size_t N>
    inline std::enable_if_t<!SIREN__LIKE_CHAR(T), Archive &> operator>>(T (&)[N]);

    template <class T>
    inline std::enable_if_t<!SIREN__LIKE_CHAR(T), Archive &> operator<<(const std::vector<T> &);

    template <class T>
    inline std::enable_if_t<!SIREN__LIKE_CHAR(T), Archive &> operator>>(std::vector<T> &);

    template <class T>
    inline std::enable_if_t<std::is_class<T>::value, Archive &> operator<<(const T &);

    template <class T>
    inline std::enable_if_t<std::is_class<T>::value, Archive &> operator>>(T &);

    inline bool isValid() const noexcept;
    inline std::size_t getNumberOfPreReadBytes() const noexcept;
    inline std::size_t getNumberOfPreWrittenBytes() const noexcept;

    explicit Archive(Stream *, std::size_t = 0, std::size_t = 0) noexcept;
    Archive(Archive &&) noexcept;
    Archive &operator=(Archive &&) noexcept;

    void serializeBytes(const void *, std::size_t);
    void deserializeBytes(void *, std::size_t);

private:
    Stream *stream_;
    std::size_t preReadByteCount_;
    std::size_t preWrittenByteCount_;

    template <class T>
    inline std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value
                            , void> serializeInteger(T);

    template <class T>
    inline std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value
                            , void> deserializeInteger(T *);

    void initialize(Stream *, std::size_t, std::size_t) noexcept;
    void move(Archive *) noexcept;
    void serializeVariableLengthInteger(std::uintmax_t);
    void deserializeVariableLengthInteger(std::uintmax_t *);
};


template <class T>
class VLI<T, true> final
{
public:
    inline VLI();
    inline VLI(T);
    inline operator const T &() const;
    inline operator T &();

private:
    T value_;
};


class ArchiveEndOfStream final
  : public std::exception
{
public:
    const char *what() const noexcept override;

private:
    explicit ArchiveEndOfStream() noexcept;

    friend Archive;
};


namespace detail {

class Serializer final
{
public:
    inline explicit Serializer(Archive *) noexcept;

    template <class T>
    inline const Serializer &operator,(const T &) const;

private:
    Archive *const archive_;

    Serializer(const Serializer &) = delete;
    Serializer &operator=(const Serializer &) = delete;
};


class Deserializer final
{
public:
    inline explicit Deserializer(Archive *) noexcept;

    template <class T>
    inline const Deserializer &operator,(T &) const;

private:
    Archive *const archive_;

    Deserializer(const Deserializer &) = delete;
    Deserializer &operator=(const Deserializer &) = delete;
};

} // namespace detail

} // namespace siren


/*
 * #include "archive-inl.h"
 */


#include <limits>

#include "assert.h"
#include "stream.h"
#include "utility.h"


namespace siren {

Archive &
Archive::operator<<(bool boolean)
{
    SIREN_ASSERT(isValid());
    operator<<(static_cast<unsigned char>(boolean));
    return *this;
}


Archive &
Archive::operator>>(bool &boolean)
{
    SIREN_ASSERT(isValid());
    unsigned char temp;
    boolean = (operator>>(temp), temp);
    return *this;
}


Archive &
Archive::operator<<(float floatingPoint)
{
    SIREN_ASSERT(isValid());
    std::uint32_t *integer;
    ConvertPointer(&floatingPoint, &integer);
    operator<<(*integer);
    return *this;
}


Archive &
Archive::operator>>(float &floatingPoint)
{
    SIREN_ASSERT(isValid());
    std::uint32_t *integer;
    ConvertPointer(&floatingPoint, &integer);
    operator>>(*integer);
    return *this;
}


Archive &
Archive::operator<<(double floatingPoint)
{
    SIREN_ASSERT(isValid());
    std::uint64_t *integer;
    ConvertPointer(&floatingPoint, &integer);
    operator<<(*integer);
    return *this;

}


Archive &
Archive::operator>>(double &floatingPoint)
{
    SIREN_ASSERT(isValid());
    std::uint64_t *integer;
    ConvertPointer(&floatingPoint, &integer);
    operator>>(*integer);
    return *this;
}


Archive &
Archive::operator<<(const std::string &string)
{
    SIREN_ASSERT(isValid());
    serializeVariableLengthInteger(string.size());
    serializeBytes(&string.front(), string.size());
    return *this;
}


Archive &
Archive::operator>>(std::string &string)
{
    SIREN_ASSERT(isValid());
    std::uintmax_t temp;
    string.resize((deserializeVariableLengthInteger(&temp), temp));
    deserializeBytes(&string.front(), string.size());
    return *this;
}


template <class T>
std::enable_if_t<std::is_integral<T>::value, Archive &>
Archive::operator<<(T integer)
{
    typedef std::make_unsigned_t<T> U;

    SIREN_ASSERT(isValid());
    serializeInteger<U>(integer);
    return *this;
}


template <class T>
std::enable_if_t<std::is_integral<T>::value, Archive &>
Archive::operator>>(T &integer)
{
    typedef std::make_unsigned_t<T> U;

    SIREN_ASSERT(isValid());
    U temp;
    deserializeInteger(&temp);
    integer = UnsignedToSigned(temp);
    return *this;
}


template <class T>
Archive &
Archive::operator<<(VLI<T> integer)
{
    SIREN_ASSERT(isValid());
    serializeVariableLengthInteger(integer);
    return *this;
}


template <class T>
Archive &
Archive::operator>>(VLI<T> &integer)
{
    SIREN_ASSERT(isValid());
    std::uintmax_t temp;
    deserializeVariableLengthInteger(&temp);
    integer = UnsignedToSigned(temp);
    return *this;
}


template <class T>
std::enable_if_t<std::is_enum<T>::value, Archive &>
Archive::operator<<(T enumerator)
{
    typedef std::underlying_type_t<T> U;

    SIREN_ASSERT(isValid());
    operator<<(static_cast<U>(enumerator));
    return *this;
}


template <class T>
std::enable_if_t<std::is_enum<T>::value, Archive &>
Archive::operator>>(T &enumerator)
{
    typedef std::underlying_type_t<T> U;

    SIREN_ASSERT(isValid());
    U temp;
    enumerator = static_cast<T>(operator>>(temp), temp);
    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<SIREN__LIKE_CHAR(T), Archive &>
Archive::operator<<(const T (&array)[N])
{
    SIREN_ASSERT(isValid());
    serializeBytes(array, N);
    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<SIREN__LIKE_CHAR(T), Archive &>
Archive::operator>>(T (&array)[N])
{
    SIREN_ASSERT(isValid());
    deserializeBytes(array, N);
    return *this;
}


template <class T>
std::enable_if_t<SIREN__LIKE_CHAR(T), Archive &>
Archive::operator<<(const std::vector<T> &vector)
{
    SIREN_ASSERT(isValid());
    serializeVariableLengthInteger(vector.size());
    serializeBytes(&vector.front(), vector.size());
    return *this;
}


template <class T>
std::enable_if_t<SIREN__LIKE_CHAR(T), Archive &>
Archive::operator>>(std::vector<T> &vector)
{
    SIREN_ASSERT(isValid());
    std::uintmax_t temp;
    vector.resize((deserializeVariableLengthInteger(&temp), temp));
    deserializeBytes(&vector.front(), vector.size());
    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<!SIREN__LIKE_CHAR(T), Archive &>
Archive::operator<<(const T (&array)[N])
{
    SIREN_ASSERT(isValid());

    for (const T &x : array) {
        operator<<(x);
    }

    return *this;
}


template <class T, std::size_t N>
std::enable_if_t<!SIREN__LIKE_CHAR(T), Archive &>
Archive::operator>>(T (&array)[N])
{
    SIREN_ASSERT(isValid());

    for (T &x : array) {
        operator>>(x);
    }

    return *this;
}


template <class T>
std::enable_if_t<!SIREN__LIKE_CHAR(T), Archive &>
Archive::operator<<(const std::vector<T> &vector)
{
    SIREN_ASSERT(isValid());
    serializeVariableLengthInteger(vector.size());

    for (const T &x : vector) {
        operator<<(x);
    }

    return *this;
}


template <class T>
std::enable_if_t<!SIREN__LIKE_CHAR(T), Archive &>
Archive::operator>>(std::vector<T> &vector)
{
    SIREN_ASSERT(isValid());
    std::uintmax_t temp;
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
    SIREN_ASSERT(isValid());
    object.serialize(this);
    return *this;
}


template <class T>
std::enable_if_t<std::is_class<T>::value, Archive &>
Archive::operator>>(T &object)
{
    SIREN_ASSERT(isValid());
    object.deserialize(this);
    return *this;
}


bool
Archive::isValid() const noexcept
{
    return stream_ != nullptr;
}

std::size_t
Archive::getNumberOfPreReadBytes() const noexcept
{
    return preReadByteCount_;
}


std::size_t
Archive::getNumberOfPreWrittenBytes() const noexcept
{
    return preWrittenByteCount_;
}


template <class T>
std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, void>
Archive::serializeInteger(T integer)
{
    constexpr unsigned int k1 = std::numeric_limits<T>::digits;
    constexpr unsigned int k2 = std::numeric_limits<unsigned char>::digits;

    stream_->reserveBuffer(preWrittenByteCount_ + sizeof(T));
    auto buffer = static_cast<unsigned char *>(stream_->getBuffer(preWrittenByteCount_));
    *buffer = integer;

    for (unsigned int n = k1 - k2; UnsignedToSigned(n) >= 1; n -= k2) {
        *++buffer = (integer >>= k2);
    }

    preWrittenByteCount_ += sizeof(T);
}


template <class T>
std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, void>
Archive::deserializeInteger(T *integer)
{
    constexpr unsigned int k1 = std::numeric_limits<T>::digits;
    constexpr unsigned int k2 = std::numeric_limits<unsigned char>::digits;

    if (stream_->getDataSize() < preReadByteCount_ + sizeof(T)) {
        throw EndOfStream();
    }

    auto data = static_cast<unsigned char *>(stream_->getData(preReadByteCount_));
    *integer = *data;

    for (unsigned int n = k2; n < k1; n += k2) {
        *integer |= static_cast<T>(*++data) << n;
    }

    preReadByteCount_ += sizeof(T);
}


template <class T>
VLI<T, true>::VLI()
{
}


template <class T>
VLI<T, true>::VLI(T value)
  : value_(value)
{
}


template <class T>
VLI<T, true>::operator const T &() const
{
    return value_;
}


template <class T>
VLI<T, true>::operator T &()
{
    return value_;
}


namespace detail {

Serializer::Serializer(Archive *archive) noexcept
  : archive_(archive)
{
}


template <class T>
const Serializer &
Serializer::operator,(const T &x) const
{
    *archive_ << x;
    return *this;
}


Deserializer::Deserializer(Archive *archive) noexcept
  : archive_(archive)
{
}


template <class T>
const Deserializer &
Deserializer::operator,(T &x) const
{
    *archive_ >> x;
    return *this;
}

} // namespace detail

} // namespace siren


#undef SIREN__LIKE_CHAR
