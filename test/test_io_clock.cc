#include <chrono>
#include <vector>

#include <unistd.h>

#include "io_clock.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Add/Remove io timers")
{
    struct Dummy : IOTimer {
    };

    IOClock ioClock;
    Dummy dummies[22];

    for (int i = 0; i < 22; ++i) {
        ioClock.addTimer(&dummies[i], std::chrono::milliseconds(5 * i));
    }

    ioClock.removeTimer(&dummies[8]);
    ioClock.removeTimer(&dummies[17]);

    std::vector<IOTimer *> timers;

    for (;;) {
        ioClock.start();
        usleep(7 * 1000);
        ioClock.stop();

        ioClock.getExpiredTimers(&timers);

        if (timers.size() == 20) {
            return;
        }
    }
}

}
