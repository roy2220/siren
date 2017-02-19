#pragma once


#include <vector>

#include <sys/epoll.h>

#include "buffer.h"
#include "hash_table.h"
#include "list.h"
#include "object_pool.h"


namespace siren {

class IOClock;
class IOWatcher;
enum class IOCondition;


namespace detail {

struct IOContext
  : HashTableNode,
    ListNode
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

    explicit IOPoller(std::size_t = 0, std::size_t = 0);
    IOPoller(IOPoller &&) noexcept;
    ~IOPoller();
    IOPoller &operator=(IOPoller &&) noexcept;

    void createContext(int);
    void destroyContext(int);
    const void *getContextTag(int) const noexcept;
    void *getContextTag(int) noexcept;
    void addWatcher(Watcher *, int, Condition) noexcept;
    void removeWatcher(Watcher *) noexcept;
    void getReadyWatchers(Clock *, std::vector<Watcher *> *);

private:
    typedef detail::IOContext Context;

    int epollFD_;
    ObjectPool<Context> contextPool_;
    HashTable contextHashTable_;
    List dirtyContextList_;
    Buffer<epoll_event> events_;

    void initialize();
    void finalize();
    void move(IOPoller *) noexcept;
    int getFD(const Context *) const noexcept;
    void setFD(Context *, int);
    void clearFD(Context *) noexcept;
#ifndef NDEBUG
    bool contextExists(int) const noexcept;
#endif
    const Context *findContext(int) const noexcept;
    Context *findContext(int) noexcept;
    void flushContexts();
};


class IOWatcher
  : private ListNode
{
protected:
    inline explicit IOWatcher() noexcept;

    ~IOWatcher() = default;

private:
    typedef IOCondition Condition;

    int fd_;
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
