#include "io_poller.h"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <algorithm>
#include <functional>
#include <limits>
#include <system_error>
#include <utility>

#include <unistd.h>

#include "io_clock.h"
#include "scope_guard.h"


namespace siren {

namespace {

const unsigned int IOEventFlags[2] = {
    EPOLLIN,
    EPOLLOUT
};

} // namespace


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


#ifndef NDEBUG
bool
IOPoller::contextExists(int fd) const noexcept
{
    return findContext(fd) != nullptr;
}
#endif


void
IOPoller::createContext(int fd)
{
    assert(isValid());
    assert(fd >= 0);
    assert(!contextExists(fd));
    auto context = contextPool_.createObject();

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        contextPool_.destroyObject(context);
    });

    setFD(context, fd);
    context->eventFlags = 0;
    context->pendingEventFlags = 0;
    context->isDirty = false;
    scopeGuard.dismiss();
}


void
IOPoller::destroyContext(int fd) noexcept
{
    assert(isValid());
    assert(contextExists(fd));
    Context *context = findContext(fd);
    clearFD(context);

    if (context->eventFlags != 0) {
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


const detail::IOContext *
IOPoller::findContext(int fd) const noexcept
{
    const HashTableNode *hashTableNode
    = contextHashTable_.search(std::hash<int>()(fd)
                               , [&] (const HashTableNode *hashTableNode) -> bool {
        auto context = static_cast<const Context *>(hashTableNode);
        return context->fd == fd;
    });

    if (hashTableNode == nullptr) {
        return nullptr;
    } else {
        auto context = static_cast<const Context *>(hashTableNode);
        return context;
    }
}


const void *
IOPoller::getContextTag(int fd) const noexcept
{
    assert(contextExists(fd));
    const Context *context = findContext(fd);
    return contextPool_.getObjectTag(context);
}


void *
IOPoller::getContextTag(int fd) noexcept
{
    assert(contextExists(fd));
    Context *context = findContext(fd);
    return contextPool_.getObjectTag(context);
}


detail::IOContext *
IOPoller::findContext(int fd) noexcept
{
    HashTableNode *hashTableNode
    = contextHashTable_.search(std::hash<int>()(fd), [&] (HashTableNode *hashTableNode) -> bool {
        auto context = static_cast<Context *>(hashTableNode);
        return context->fd == fd;
    });

    if (hashTableNode == nullptr) {
        return nullptr;
    } else {
        auto context = static_cast<Context *>(hashTableNode);
        return context;
    }
}


void
IOPoller::addWatcher(Watcher *watcher, int fd, Condition condition) noexcept
{
    assert(isValid());
    assert(watcher != nullptr);
    assert(fd >= 0);
    assert(contextExists(fd));
    Context *context = findContext(watcher->fd_ = fd);
    auto i = static_cast<std::size_t>(watcher->condition_ = condition);
    context->watcherLists[i].appendNode(watcher);

    if (watcher->isOnly()) {
        context->pendingEventFlags |= IOEventFlags[i];

        if (!context->isDirty) {
            dirtyContextList_.appendNode((context->isDirty = true, context));
        }
    }
}


void
IOPoller::removeWatcher(Watcher *watcher) noexcept
{
    assert(isValid());
    assert(watcher != nullptr);
    assert(contextExists(watcher->fd_));
    Context *context = findContext(watcher->fd_);
#ifndef NDEBUG
    watcher->fd_ = -1;
#endif
    auto i = static_cast<std::size_t>(watcher->condition_);
    watcher->remove();

    if (context->watcherLists[i].isEmpty()) {
        context->pendingEventFlags &= ~IOEventFlags[i];

        if (!context->isDirty) {
            dirtyContextList_.appendNode((context->isDirty = true, context));
        }
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

        if (context->eventFlags != context->pendingEventFlags) {
            int op;

            if (context->eventFlags == 0) {
                op = EPOLL_CTL_ADD;
            } else {
                if (context->pendingEventFlags == 0) {
                    op = EPOLL_CTL_DEL;
                } else {
                    op = EPOLL_CTL_MOD;
                }
            }

            epoll_event event;
            event.events = context->pendingEventFlags | EPOLLET;
            event.data.ptr = context;

            if (epoll_ctl(epollFD_, op, getFD(context), &event) < 0) {
                throw std::system_error(errno, std::system_category(), "epoll_ctl() failed");
            }

            context->eventFlags = context->pendingEventFlags;
        }

        context->isDirty = false;
    }

    scopeGuard.dismiss();
}


void
IOPoller::getReadyWatchers(Clock *clock, std::vector<Watcher *> *watchers)
{
    assert(isValid());
    assert(clock != nullptr);
    assert(watchers != nullptr);
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

        if ((event->events & (IOEventFlags[0] | EPOLLERR | EPOLLHUP)) != 0) {
            SIREN_LIST_FOREACH_REVERSE(listNode, context->watcherLists[0]) {
                auto watcher = static_cast<Watcher *>(listNode);
                watchers->push_back(watcher);
            }
        }

        if ((event->events & (IOEventFlags[1] | EPOLLERR | EPOLLHUP)) != 0) {
            SIREN_LIST_FOREACH_REVERSE(listNode, context->watcherLists[1]) {
                auto watcher = static_cast<Watcher *>(listNode);
                watchers->push_back(watcher);
            }
        }
    }
}

} // namespace siren
