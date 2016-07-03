#pragma once


#include <cstddef>
#include <vector>


namespace siren {

class Stream final
{
public:
    inline explicit Stream();

    inline const void *getData(std::size_t = 0) const;
    inline void *getData(std::size_t = 0);
    inline std::size_t getDataSize() const;
    inline void pickData(std::size_t);
    inline void dropData(std::size_t);
    inline void *getBuffer(std::size_t = 0);
    inline std::size_t getBufferSize() const;
    inline void growBuffer(std::size_t);

private:
    std::vector<char> base_;
    std::size_t rSize_;
    std::size_t wSize_;

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
  : rSize_(0),
    wSize_(0)
{
}


const void *
Stream::getData(std::size_t offset) const
{
    return base_.data() + rSize_ + offset;
}


void *
Stream::getData(std::size_t offset)
{
    return base_.data() + rSize_ + offset;
}


std::size_t
Stream::getDataSize() const
{
    return wSize_ - rSize_;
}


void
Stream::pickData(std::size_t size)
{
    assert(wSize_ + size <= base_.size());
    wSize_ += size;
}


void
Stream::dropData(std::size_t size)
{
    assert(rSize_ + size <= wSize_);
    rSize_ += size;

    if (rSize_ >= wSize_ - rSize_) {
        std::memcpy(base_.data(), base_.data() + rSize_, wSize_ - rSize_);
        wSize_ -= rSize_;
        rSize_ = 0;
    }
}


void *
Stream::getBuffer(std::size_t offset)
{
    return base_.data() + wSize_ + offset;
}


std::size_t
Stream::getBufferSize() const
{
    return base_.size() - wSize_;
}


void
Stream::growBuffer(std::size_t size)
{
    base_.resize(NextPowerOfTwo(base_.size() + size));
}

}
