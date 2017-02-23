#pragma once


#include <cstddef>
#include <array>
#include <vector>

#include <sys/epoll.h>

#include "buffer.h"
#include "enum_class_as_flag.h"
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
    typedef IOCondition Condition;

    static std::array<IOCondition, 4> Conditions;

    int fd;
    Condition conditions;
    Condition pendingConditions;
    bool isDirty;
    List watcherList;
    std::array<std::size_t, Conditions.size()> watcherCounts;
};

} // namespace detail


class IOPoller final
{
public:
    typedef IOWatcher Watcher;
    typedef IOCondition Condition;
    typedef IOClock Clock;

    inline bool isValid() const noexcept;
    inline bool contextExists(int) const noexcept;

    explicit IOPoller(std::size_t = 0, std::size_t = 0);
    IOPoller(IOPoller &&) noexcept;
    ~IOPoller();
    IOPoller &operator=(IOPoller &&) noexcept;

    void createContext(int);
    void destroyContext(int) noexcept;
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
    void finalize() noexcept;
    void move(IOPoller *) noexcept;
    int getFD(const Context *) const noexcept;
    void setFD(Context *, int);
    void clearFD(Context *) noexcept;
    const Context *findContext(int) const noexcept;
    Context *findContext(int) noexcept;
    void flushContexts();
};


class IOWatcher
  : private ListNode
{
public:
    typedef IOCondition Condition;

    inline Condition getReadyConditions() const noexcept;

protected:
    inline explicit IOWatcher() noexcept;

    ~IOWatcher() = default;

private:
    typedef detail::IOContext Context;

    Context *context_;
    Condition conditions_;
    Condition readyConditions_;

    IOWatcher(const IOWatcher &) = delete;
    IOWatcher &operator=(const IOWatcher &) = delete;

    friend IOPoller;
};


enum class IOCondition
{
    No = 0,
    In = EPOLLIN,
    Out = EPOLLOUT,
    RdHup = EPOLLRDHUP,
    Pri = EPOLLPRI,
    Err = EPOLLERR,
    Hup = EPOLLHUP,
};


SIREN_ENUM_CLASS_AS_FLAG(IOCondition)

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


bool
IOPoller::contextExists(int fd) const noexcept
{
    return findContext(fd) != nullptr;
}


IOWatcher::IOWatcher() noexcept
#ifndef NDEBUG
  : context_(nullptr)
#endif
{
}


IOCondition
IOWatcher::getReadyConditions() const noexcept
{
    return readyConditions_;
}

} // namespace siren
