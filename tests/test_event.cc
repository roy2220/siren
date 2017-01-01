#include "event.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Wait for/Trigger events")
{
    Scheduler sched;
    Event e(&sched);
    int s = 0;

    for (int i = 0; i < 3; ++i) {
        sched.createFiber([&sched, &e, &s] () -> void {
            while (s == 0) {
                e.waitFor();
            }

            s = 0;
        });
    }

    sched.run();
    SIREN_TEST_ASSERT(sched.getNumberOfAliveFibers() == 3);
    s = 1;
    e.trigger();
    sched.run();
    SIREN_TEST_ASSERT(sched.getNumberOfAliveFibers() == 2);
    s = 2;
    e.trigger();
    sched.run();
    SIREN_TEST_ASSERT(sched.getNumberOfAliveFibers() == 1);
    s = 3;
    e.trigger();
    sched.run();
    SIREN_TEST_ASSERT(sched.getNumberOfAliveFibers() == 0);
}

}
