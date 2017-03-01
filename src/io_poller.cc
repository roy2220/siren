#include "io_poller.h"

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <algorithm>
#include <functional>
#include <limits>
#include <system_error>
#include <utility>

#include <unistd.h>

#include "assert.h"
#include "io_clock.h"
#include "scope_guard.h"


namespace siren {

IOPoller::IOPoller(std::size_t contextTagAlignment, std::size_t contextTagSize)
  : contextPool_(64, contextTagAlignment, contextTagSize)
{
    initialize();
}


IOPoller::IOPoller(IOPoller &&other) noexcept
  : contextPool_(std::move(other.contextPool_)),
    contextHashTable_(std::move(other.contextHashTable_)),
    dirtyContextList_(std::move(other.dirtyContextList_)),
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
        contextPool_ = std::move(other.contextPool_);
        contextHashTable_ = std::move(other.contextHashTable_);
        dirtyContextList_ = std::move(other.dirtyContextList_);
        events_ = std::move(other.events_);
        other.move(this);
    }

    return *this;
}


void
IOPoller::initialize()
{
    events_.setLength(64);
    epollFD_ = epoll_create1(0);

    if (epollFD_ < 0) {
        throw std::system_error(errno, std::system_category(), "epoll_create1() failed");
    }
}


void
IOPoller::finalize() noexcept
{
    if (isValid()) {
        if (close(epollFD_) < 0 && errno != EINTR) {
            std::perror("close() failed");
            std::terminate();
        }

        contextHashTable_.traverse([&] (HashTableNode *hashTableNode) -> void {
            auto context = static_cast<Context *>(hashTableNode);
            contextPool_.destroyObject(context);
        });
    }
}


void
IOPoller::move(IOPoller *other) noexcept
{
    other->epollFD_ = epollFD_;
    epollFD_ = -1;
}


int
IOPoller::getFD(const Context *context) const noexcept
{
    return context->fd;
}


void
IOPoller::setFD(Context *context, int fd)
{
    contextHashTable_.insertNode(context, std::hash<int>()(context->fd = fd));
}


void
IOPoller::clearFD(Context *context) noexcept
{
    contextHashTable_.removeNode(context);
}


void
IOPoller::createContext(int fd)
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(fd >= 0);
    SIREN_ASSERT(!contextExists(fd));
    auto context = contextPool_.createObject();

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        contextPool_.destroyObject(context);
    });

    setFD(context, fd);
    context->conditions = Condition::No;
    context->pendingConditions = Condition::No;
    context->isDirty = false;

    for (std::size_t &watcherCount : context->watcherCounts) {
        watcherCount = 0;
    }

    scopeGuard.dismiss();
}


void
IOPoller::destroyContext(int fd) noexcept
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(contextExists(fd));
    Context *context = findContext(fd);
    clearFD(context);

    if (context->conditions != Condition::No) {
        if (epoll_ctl(epollFD_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
            std::perror("epoll_ctl() failed");
            std::terminate();
        }
    }

    if (context->isDirty) {
        context->remove();
    }

    contextPool_.destroyObject(context);
}


const void *
IOPoller::getContextTag(int fd) const noexcept
{
    SIREN_ASSERT(contextExists(fd));
    const Context *context = findContext(fd);
    return contextPool_.getObjectTag(context);
}


void *
IOPoller::getContextTag(int fd) noexcept
{
    SIREN_ASSERT(contextExists(fd));
    Context *context = findContext(fd);
    return contextPool_.getObjectTag(context);
}


const detail::IOContext *
IOPoller::findContext(int fd) const noexcept
{
    const HashTableNode *hashTableNode = contextHashTable_.search(
        std::hash<int>()(fd),

        [&] (const HashTableNode *hashTableNode) -> bool {
            auto context = static_cast<const Context *>(hashTableNode);
            return context->fd == fd;
        }
    );

    if (hashTableNode == nullptr) {
        return nullptr;
    } else {
        auto context = static_cast<const Context *>(hashTableNode);
        return context;
    }
}


detail::IOContext *
IOPoller::findContext(int fd) noexcept
{
    HashTableNode *hashTableNode = contextHashTable_.search(
        std::hash<int>()(fd),

        [&] (HashTableNode *hashTableNode) -> bool {
            auto context = static_cast<Context *>(hashTableNode);
            return context->fd == fd;
        }
    );

    if (hashTableNode == nullptr) {
        return nullptr;
    } else {
        auto context = static_cast<Context *>(hashTableNode);
        return context;
    }
}


void
IOPoller::addWatcher(Watcher *watcher, int fd, Condition conditions) noexcept
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(watcher != nullptr);
    SIREN_ASSERT(watcher->context_ == nullptr);
    SIREN_ASSERT(fd >= 0);
    SIREN_ASSERT(contextExists(fd));
    Context *context = findContext(fd);
    (watcher->context_ = context)->watcherList.appendNode(watcher);
    watcher->conditions_ = conditions | Condition::Err | Condition::Hup;
    std::size_t *watcherCount = context->watcherCounts;
    bool contextIsModified = false;

    for (Condition condition : {SIREN__IO_CONDITIONS}) {
        if ((conditions & condition) == condition) {
            if (++*watcherCount == 1) {
                context->pendingConditions |= condition;
                contextIsModified = true;
            }
        }

        ++watcherCount;
    }

    if (contextIsModified && !context->isDirty) {
        dirtyContextList_.appendNode((context->isDirty = true, context));
    }
}


void
IOPoller::removeWatcher(Watcher *watcher) noexcept
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(watcher != nullptr);
    SIREN_ASSERT(watcher->context_ != nullptr);
    Context *context = watcher->context_;
#ifdef SIREN_WITH_DEBUG
    watcher->context_ = nullptr;
#endif
    watcher->remove();
    std::size_t *watcherCount = context->watcherCounts;
    bool contextIsModified = false;

    for (Condition condition : {SIREN__IO_CONDITIONS}) {
        if ((watcher->conditions_ & condition) == condition) {
            if (--*watcherCount == 0) {
                context->pendingConditions &= ~condition;
                contextIsModified = true;
            }
        }

        ++watcherCount;
    }

    if (contextIsModified && !context->isDirty) {
        dirtyContextList_.appendNode((context->isDirty = true, context));
    }
}


void
IOPoller::flushContexts()
{
    List list = std::move(dirtyContextList_);
    Context *context;

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        dirtyContextList_.prependNodes(list.getHead(), context);
    });

    SIREN_LIST_FOREACH_REVERSE(listNode, list) {
        context = static_cast<Context *>(listNode);

        if (context->conditions != context->pendingConditions) {
            int op;

            if (context->conditions == Condition::No) {
                op = EPOLL_CTL_ADD;
            } else {
                if (context->pendingConditions == Condition::No) {
                    op = EPOLL_CTL_DEL;
                } else {
                    op = EPOLL_CTL_MOD;
                }
            }

            epoll_event event;
            event.events = static_cast<int>(context->pendingConditions) | EPOLLET;
            event.data.ptr = context;

            if (epoll_ctl(epollFD_, op, getFD(context), &event) < 0) {
                throw std::system_error(errno, std::system_category(), "epoll_ctl() failed");
            }

            context->conditions = context->pendingConditions;
        }

        context->isDirty = false;
    }

    scopeGuard.dismiss();
}


void
IOPoller::getReadyWatchers(Clock *clock, std::vector<Watcher *> *watchers)
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(clock != nullptr);
    SIREN_ASSERT(watchers != nullptr);
    flushContexts();
    std::size_t eventCount = 0;
    clock->start();
    int timeout = std::min(clock->getDueTime()
                           , std::chrono::milliseconds(std::numeric_limits<int>::max())).count();

    for (;;) {
        int numberOfEvents = epoll_wait(epollFD_, events_ + eventCount
                                        , events_.getLength() - eventCount, timeout);

        if (numberOfEvents < 0) {
            if (errno != EINTR) {
                clock->stop();
                throw std::system_error(errno, std::system_category(), "epoll_wait() failed");
            }

            clock->restart();
            timeout = std::min(clock->getDueTime()
                               , std::chrono::milliseconds(std::numeric_limits<int>::max()))
                      .count();
        } else {
            clock->stop();
            eventCount += numberOfEvents;

            if (eventCount < events_.getLength()) {
                break;
            } else {
                events_.setLength(eventCount + 1);
                clock->start();
                timeout = 0;
            }
        }
    }

    for (std::size_t i = 0; i < eventCount; ++i) {
        epoll_event *event = &events_[i];
        auto context = static_cast<Context *>(event->data.ptr);

        SIREN_LIST_FOREACH_REVERSE(listNode, context->watcherList) {
            auto watcher = static_cast<Watcher *>(listNode);
            Condition readyConditions
                      = static_cast<Condition>(event->events
                                               & static_cast<int>(watcher->conditions_));

            if (readyConditions != Condition::No) {
                watcher->readyConditions_ = readyConditions;
                watchers->push_back(watcher);
            }
        }
    }
}

} // namespace siren
