#include "io_poller.h"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <limits>
#include <system_error>

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


IOPoller::IOPoller()
  : objectPool_(64)
{
    initialize();
}


IOPoller::IOPoller(IOPoller &&other) noexcept
  : objectPool_(std::move(other.objectPool_)),
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
        objectPool_ = std::move(other.objectPool_);
        objects_ = std::move(other.objects_);
        dirtyObjectList_ = std::move(other.dirtyObjectList_);
        events_ = std::move(other.events_);
        other.move(this);
    }

    return *this;
}


void
IOPoller::initialize()
{
    epollFD_ = epoll_create1(0);

    if (epollFD_ < 0) {
        throw std::system_error(errno, std::system_category(), "epoll_create1() failed");
    }

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        if (close(epollFD_) < 0 && errno != EINTR) {
            throw std::system_error(errno, std::system_category(), "close() failed");
        }
    });

    events_.setLength(64);
    scopeGuard.dismiss();
}


void
IOPoller::finalize()
{
    if (isValid()) {
        if (close(epollFD_) < 0 && errno != EINTR) {
            throw std::system_error(errno, std::system_category(), "close() failed");
        }

        for (Object *object : objects_) {
            if (object != nullptr) {
                objectPool_.destroyObject(object);
            }
        }
    }
}


void
IOPoller::move(IOPoller *other) noexcept
{
    other->epollFD_ = epollFD_;
    epollFD_ = -1;
}


#ifndef NDEBUG
bool
IOPoller::objectExists(int objectFD) const noexcept
{
    return static_cast<std::size_t>(objectFD) < objects_.size() && objects_[objectFD] != nullptr;
}
#endif


void
IOPoller::createObject(int objectFD)
{
    assert(isValid());
    assert(objectFD >= 0);
    assert(!objectExists(objectFD));

    if (static_cast<std::size_t>(objectFD) >= objects_.size()) {
        objects_.resize(objectFD + 1, nullptr);
    }

    auto object = objectPool_.createObject();
    object->eventFlags = 0;
    object->pendingEventFlags = 0;
    object->isDirty = false;
    objects_[object->fd = objectFD] = object;
}


void
IOPoller::destroyObject(int objectFD)
{
    assert(isValid());
    assert(objectFD >= 0);
    assert(objectExists(objectFD));
    Object *object = objects_[objectFD];

    if (object->eventFlags != 0) {
        if (epoll_ctl(epollFD_, EPOLL_CTL_DEL, objectFD, nullptr) < 0) {
            throw std::system_error(errno, std::system_category(), "epoll_ctl() failed");
        }
    }

    if (object->isDirty) {
        object->remove();
    }

    objects_[objectFD] = nullptr;
    objectPool_.destroyObject(object);
}


void
IOPoller::addWatcher(Watcher *watcher, int objectFD, Condition condition) noexcept
{
    assert(isValid());
    assert(watcher != nullptr);
    assert(objectFD >= 0);
    assert(objectExists(objectFD));
    Object *object = objects_[watcher->objectFD_ = objectFD];
    auto i = static_cast<std::size_t>(watcher->condition_ = condition);
    object->watcherLists[i].appendNode(watcher);

    if (watcher->isOnly()) {
        object->pendingEventFlags |= IOEventFlags[i];

        if (!object->isDirty) {
            object->isDirty = true;
            dirtyObjectList_.appendNode(object);
        }
    }
}


void
IOPoller::removeWatcher(Watcher *watcher) noexcept
{
    assert(isValid());
    assert(watcher != nullptr);
    assert(watcher->objectFD_ >= 0);
    assert(objectExists(watcher->objectFD_));
    Object *object = objects_[watcher->objectFD_];
    auto i = static_cast<std::size_t>(watcher->condition_);
    watcher->remove();

    if (object->watcherLists[i].isEmpty()) {
        object->pendingEventFlags &= ~IOEventFlags[i];

        if (!object->isDirty) {
            dirtyObjectList_.appendNode((object->isDirty = true, object));
        }
    }
}


void
IOPoller::flushObjects()
{
    SIREN_LIST_FOREACH_REVERSE(listNode, dirtyObjectList_) {
        auto object = static_cast<Object *>(listNode);

        if (object->eventFlags != object->pendingEventFlags) {
            int op;

            if (object->eventFlags == 0) {
                op = EPOLL_CTL_ADD;
            } else {
                if (object->pendingEventFlags == 0) {
                    op = EPOLL_CTL_DEL;
                } else {
                    op = EPOLL_CTL_MOD;
                }
            }

            epoll_event event;
            event.events = object->pendingEventFlags | EPOLLET;
            event.data.ptr = object;

            if (epoll_ctl(epollFD_, op, object->fd, &event) < 0) {
                throw std::system_error(errno, std::system_category(), "epoll_ctl() failed");
            }

            object->eventFlags = object->pendingEventFlags;
        }

        object->isDirty = false;
    }

    dirtyObjectList_.reset();
}


void
IOPoller::getReadyWatchers(Clock *clock, std::vector<Watcher *> *watchers)
{
    assert(isValid());
    assert(clock != nullptr);
    assert(watchers != nullptr);
    flushObjects();
    std::size_t eventCount = 0;
    clock->start();
    int timeout = clock->getDueTime().count();

    for (;;) {
        int numberOfEvents = epoll_wait(epollFD_, events_ + eventCount
                                        , events_.getLength() - eventCount, timeout);

        if (numberOfEvents < 0) {
            if (errno != EINTR) {
                clock->stop();
                throw std::system_error(errno, std::system_category(), "epoll_wait() failed");
            }

            clock->restart();
            timeout = clock->getDueTime().count();
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
        auto object = static_cast<Object *>(event->data.ptr);

        if ((event->events & (IOEventFlags[0] | EPOLLERR | EPOLLHUP)) != 0) {
            SIREN_LIST_FOREACH_REVERSE(listNode, object->watcherLists[0]) {
                auto watcher = static_cast<Watcher *>(listNode);
                watchers->push_back(watcher);
            }
        }

        if ((event->events & (IOEventFlags[1] | EPOLLERR | EPOLLHUP)) != 0) {
            SIREN_LIST_FOREACH_REVERSE(listNode, object->watcherLists[1]) {
                auto watcher = static_cast<Watcher *>(listNode);
                watchers->push_back(watcher);
            }
        }
    }
}

} // namespace siren
