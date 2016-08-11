#include <string>

#include "scheduler.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Fibers yield")
{
    Scheduler scheduler;
    std::string s;

    scheduler.createFiber([&scheduler, &s] () -> void {
        for (int i = 0; i < 3; ++i) {
            s.push_back('a');
            scheduler.currentFiberYields();
        }

        scheduler.currentFiberExits();
    });

    scheduler.createFiber([&scheduler, &s] () -> void {
        for (int i = 0; i < 3; ++i) {
            s.push_back('b');
            scheduler.currentFiberYields();
        }

        scheduler.currentFiberExits();
    });

    scheduler.createFiber([&scheduler, &s] () -> void {
        for (int i = 0; i < 3; ++i) {
            s.push_back('c');
            scheduler.currentFiberYields();
        }

        scheduler.currentFiberExits();
    });

    scheduler.run();
    SIREN_TEST_ASSERT(s == "abcabcabc");
}


SIREN_TEST("Suspend/Resume fibers")
{
    Scheduler scheduler;
    void *fh;
    int s = 0;

    scheduler.createFiber([&scheduler, &s, &fh] () -> void {
        fh = scheduler.getCurrentFiber();
        ++s;
        scheduler.suspendCurrentFiber();
        ++s;
        scheduler.currentFiberExits();
    });

    scheduler.run();
    SIREN_TEST_ASSERT(s == 1);
    scheduler.resumeFiber(fh);
    scheduler.run();
    SIREN_TEST_ASSERT(s == 2);
}

}
