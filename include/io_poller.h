#pragma once


#include <vector>

#include <sys/epoll.h>

#include "buffer.h"
#include "list.h"
#include "object_pool.h"


namespace siren {

class IOClock;
class IOWatcher;
enum class IOCondition;


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

} // namespace detail


class IOPoller final
{
public:
    typedef IOWatcher Watcher;
    typedef IOCondition Condition;
    typedef IOClock Clock;

    inline bool isValid() const noexcept;

    explicit IOPoller();
    IOPoller(IOPoller &&) noexcept;
    ~IOPoller();
    IOPoller &operator=(IOPoller &&) noexcept;

    void createObject(int);
    void destroyObject(int);
    void addWatcher(Watcher *, int, Condition) noexcept;
    void removeWatcher(Watcher *) noexcept;
    void getReadyWatchers(Clock *, std::vector<Watcher *> *);

private:
    typedef detail::IOObject Object;

    int epollFD_;
    ObjectPool<Object> objectPool_;
    std::vector<detail::IOObject *> objects_;
    List dirtyObjectList_;
    Buffer<epoll_event> events_;

    void initialize();
    void finalize();
    void move(IOPoller *) noexcept;
#ifndef NDEBUG
    bool objectExists(int) const noexcept;
#endif
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

    int objectFD_;
    Condition condition_;

    IOWatcher(const IOWatcher &) = delete;
    IOWatcher &operator=(const IOWatcher &) = delete;

    friend IOPoller;
};


enum class IOCondition
{
    Readable = 0,
    Writable,
};

} // namespace siren


/*
 * #include "io_poller-inl.h"
 */


namespace siren {

bool
IOPoller::isValid() const noexcept
{
    return epollFD_ >= 0;
}


IOWatcher::IOWatcher() noexcept
{
}

} // namespace siren
