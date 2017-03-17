#include <chrono>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include "assert.h"
#include "io_clock.h"
#include "io_poller.h"
#include "test.h"
#include "utility.h"


namespace {

using namespace siren;


SIREN_TEST("Add/Remove io watchers")
{
    struct IOTimerContext : IOTimer {
    };

    struct IOWatcherContext : IOWatcher {
    };

    int r;
    SIREN_UNUSED(r);
    int fds[2];
    r = pipe2(fds, O_NONBLOCK);
    SIREN_ASSERT(r == 0);
    IOClock ioClock;
    IOTimerContext ioTimerContext;
    ioClock.addTimer(&ioTimerContext, std::chrono::milliseconds(100));
    IOPoller ioPoller;
    IOWatcherContext ioWatcherContext;
    ioPoller.createContext(fds[0]);
    ioPoller.addWatcher(&ioWatcherContext, fds[0], IOCondition::In);
    std::vector<IOWatcher *> readyWatchers;

    ioPoller.getReadyWatchers(&ioClock, [&] (IOWatcher *x, IOCondition) -> void {
        readyWatchers.push_back(x);
    });

    SIREN_TEST_ASSERT(readyWatchers.size() == 0);
    std::vector<IOTimer *> expiredTimers;

    ioClock.removeExpiredTimers([&] (IOTimer *x) -> void {
        expiredTimers.push_back(x);
    });

    SIREN_TEST_ASSERT(expiredTimers.size() == 1);
    SIREN_TEST_ASSERT(expiredTimers[0] == &ioTimerContext);
    char c;
    r = read(fds[0], &c, 1);
    SIREN_ASSERT(r == -1);

    std::thread t([fds] {
        usleep(100 * 1000);
        char c = 'a';
        int r = write(fds[1], &c, 1);
        SIREN_UNUSED(r);
        SIREN_ASSERT(r == 1);
    });

    ioPoller.getReadyWatchers(&ioClock, [&] (IOWatcher *x, IOCondition) -> void {
        readyWatchers.push_back(x);
    });

    SIREN_TEST_ASSERT(readyWatchers.size() == 1);
    SIREN_TEST_ASSERT(readyWatchers[0] == &ioWatcherContext);
    t.join();
}

}
