#pragma once


#include <cstddef>

#include <sys/epoll.h>

#include "buffer.h"
#include "config.h"
#include "enum_class_as_flag.h"
#include "hash_table.h"
#include "list.h"
#include "object_pool.h"


#define SIREN__IO_CONDITIONS ::siren::IOCondition::In, ::siren::IOCondition::Out \
                             , ::siren::IOCondition::RdHup, ::siren::IOCondition::Pri
#define SIREN__NUMBER_OF_IO_CONDITIONS 4


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

    int fd;
    Condition conditions;
    Condition pendingConditions;
    bool isDirty;
    List watcherList;
    std::size_t watcherCounts[SIREN__NUMBER_OF_IO_CONDITIONS];
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

    template <class T>
    inline void getReadyWatchers(Clock *, T &&);

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
    std::size_t pollEvents(Clock *);
};


class IOWatcher
  : private ListNode
{
public:
    typedef IOCondition Condition;

protected:
    inline explicit IOWatcher() noexcept;

    ~IOWatcher() = default;

private:
    typedef detail::IOContext Context;

    Context *context_;
    Condition conditions_;

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


#include "assert.h"


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


template <class T>
void
IOPoller::getReadyWatchers(Clock *clock, T &&callback)
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(clock != nullptr);
    flushContexts();
    std::size_t numberOfEvents = pollEvents(clock);

    for (std::size_t i = 0; i < numberOfEvents; ++i) {
        epoll_event *event = &events_[i];
        auto context = static_cast<Context *>(event->data.ptr);

        SIREN_LIST_FOREACH_REVERSE(listNode, context->watcherList) {
            auto watcher = static_cast<Watcher *>(listNode);
            Condition readyConditions
                      = static_cast<Condition>(event->events
                                               & static_cast<int>(watcher->conditions_));

            if (readyConditions != Condition::No) {
                callback(watcher, readyConditions);
            }
        }
    }
}


IOWatcher::IOWatcher() noexcept
#ifdef SIREN_WITH_DEBUG
  : context_(nullptr)
#endif
{
}

} // namespace siren
