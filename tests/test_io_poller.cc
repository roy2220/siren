#include <chrono>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

#include "io_clock.h"
#include "io_poller.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Add/Remove io watchers")
{
    struct IOTimerContext : IOTimer {
    };

    struct IOWatcherContext : IOWatcher {
    };

    int fds[2];
    assert(pipe2(fds, O_NONBLOCK) == 0);
    IOClock ioClock;
    IOTimerContext ioTimerContext;
    ioClock.addTimer(&ioTimerContext, std::chrono::milliseconds(500));
    IOPoller ioPoller;
    IOWatcherContext ioWatcherContext;
    ioPoller.createObject(fds[0]);
    ioPoller.addWatcher(&ioWatcherContext, fds[0], IOCondition::Readable);
    std::vector<IOWatcher *> readyWatchers;
    ioPoller.getReadyWatchers(&ioClock, &readyWatchers);
    SIREN_TEST_ASSERT(readyWatchers.size() == 0);
    std::vector<IOTimer *> expiredTimers;
    ioClock.getExpiredTimers(&expiredTimers);
    SIREN_TEST_ASSERT(expiredTimers.size() == 1);
    SIREN_TEST_ASSERT(expiredTimers[0] == &ioTimerContext);
    char c;
    assert(read(fds[0], &c, 1) == -1);

    std::thread t([fds] {
        usleep(500 * 1000);
        char c = 'a';
        assert(write(fds[1], &c, 1) == 1);
    });

    ioPoller.getReadyWatchers(&ioClock, &readyWatchers);
    SIREN_TEST_ASSERT(readyWatchers.size() == 1);
    SIREN_TEST_ASSERT(readyWatchers[0] == &ioWatcherContext);
    t.join();
}

}
