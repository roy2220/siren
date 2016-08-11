#pragma once


#include <vector>

#include <sys/epoll.h>

#include "buffer.h"
#include "list.h"
#include "memory_pool.h"


namespace siren {

enum class IOCondition;

class IOWatcher;
class IOClock;


namespace detail {

struct IOObject
  : ListNode
{
    int fd;
    int eventFlags;
    int pendingEventFlags;
    bool isDirty;
    List watcherLists[2];
};

}


class IOPoller final
{
public:
    typedef IOWatcher Watcher;
    typedef IOCondition Condition;
    typedef IOClock Clock;

    inline explicit IOPoller();
    inline IOPoller(IOPoller &&) noexcept;
    inline ~IOPoller();
    inline IOPoller &operator=(IOPoller &&) noexcept;

    void createObject(int);
    void destroyObject(int);
    void addWatcher(Watcher *, int, Condition) noexcept;
    void removeWatcher(Watcher *) noexcept;
    void getReadyWatchers(Clock *, std::vector<Watcher *> *);

private:
    typedef detail::IOObject Object;

    int fd_;
    MemoryPool objectMemoryPool_;
    std::vector<detail::IOObject *> objects_;
    List dirtyObjectList_;
    Buffer<epoll_event> events_;

    inline void move(IOPoller *) noexcept;
#ifndef NDEBUG
    inline bool objectExists(int) const noexcept;
#endif

    void initialize();
    void finalize();
    void flushObjects();
};


class IOWatcher
  : private ListNode
{
protected:
    inline explicit IOWatcher() noexcept;

    ~IOWatcher() = default;

private:
    typedef IOCondition Condition;

    int objectFd_;
    Condition condition_;

    IOWatcher(const IOWatcher &) = delete;
    IOWatcher &operator=(const IOWatcher &) = delete;

    friend IOPoller;
};


enum class IOCondition
{
    Readable = 0,
    Writable
};

}


/*
 * #include "io_poller-inl.h"
 */


namespace siren {

IOPoller::IOPoller()
  : objectMemoryPool_(alignof(Object), sizeof(Object), 64)
{
    initialize();
}


IOPoller::IOPoller(IOPoller &&other) noexcept
  : objectMemoryPool_(std::move(other.objectMemoryPool_)),
    objects_(std::move(other.objects_)),
    dirtyObjectList_(std::move(other.dirtyObjectList_)),
    events_(std::move(other.events_))
{
    other.move(this);
}


IOPoller::~IOPoller()
{
    finalize();
}


IOPoller &
IOPoller::operator=(IOPoller &&other) noexcept
{
    if (&other != this) {
        finalize();
        objectMemoryPool_ = std::move(other.objectMemoryPool_);
        objects_ = std::move(other.objects_);
        dirtyObjectList_ = std::move(other.dirtyObjectList_);
        events_ = std::move(other.events_);
        other.move(this);
    }

    return *this;
}


void
IOPoller::move(IOPoller *other) noexcept
{
    other->fd_ = fd_;
    fd_ = -1;
}


#ifndef NDEBUG
bool
IOPoller::objectExists(int objectFd) const noexcept
{
    return static_cast<std::size_t>(objectFd) < objects_.size() && objects_[objectFd] != nullptr;
}
#endif


IOWatcher::IOWatcher() noexcept
{
}

}
