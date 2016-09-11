#include <string>

#include "scheduler.h"
#include "scope_guard.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Interrupt fibers")
{
    int i = 3;

    {
        Scheduler scheduler;

        void *fh = scheduler.createFiber([&scheduler, &i] () -> void {
            auto sg = MakeScopeGuard([&i] () -> void { --i; });
            scheduler.suspendFiber(scheduler.getCurrentFiber());
        });

        scheduler.run();
        scheduler.interruptFiber(fh);
        SIREN_TEST_ASSERT(i == 2);

        scheduler.createFiber([&scheduler, &i] () -> void {
            auto sg = MakeScopeGuard([&i] () -> void { --i; });
            scheduler.suspendFiber(scheduler.getCurrentFiber());
        });

        scheduler.createFiber([&scheduler, &i] () -> void {
            auto sg = MakeScopeGuard([&i] () -> void { --i; });
            scheduler.suspendFiber(scheduler.getCurrentFiber());
        });

        scheduler.run();
        SIREN_TEST_ASSERT(scheduler.hasAliveFibers());
    }

    SIREN_TEST_ASSERT(i == 0);
}


SIREN_TEST("Fibers yield")
{
    Scheduler scheduler;
    std::string s;

    scheduler.createFiber([&scheduler, &s] () -> void {
        for (int i = 0; i < 3; ++i) {
            s.push_back('a');
            scheduler.yieldTo();
        }
    });

    scheduler.createFiber([&scheduler, &s] () -> void {
        for (int i = 0; i < 3; ++i) {
            s.push_back('b');
            scheduler.yieldTo();
        }
    });

    scheduler.createFiber([&scheduler, &s] () -> void {
        for (int i = 0; i < 3; ++i) {
            s.push_back('c');
            scheduler.yieldTo();
        }
    });

    scheduler.run();
    SIREN_TEST_ASSERT(!scheduler.hasAliveFibers());
    SIREN_TEST_ASSERT(s == "cbacbacba");
}


SIREN_TEST("Suspend/Resume fibers")
{
    Scheduler scheduler;
    int s = 0;

    void *fh = scheduler.createFiber([&scheduler, &s] () -> void {
        ++s;
        scheduler.suspendFiber(scheduler.getCurrentFiber());
        ++s;
    });

    scheduler.suspendFiber(fh);
    scheduler.run();
    SIREN_TEST_ASSERT(!scheduler.hasAliveFibers());
    SIREN_TEST_ASSERT(s == 0);
    scheduler.resumeFiber(fh);
    scheduler.run();
    SIREN_TEST_ASSERT(scheduler.hasAliveFibers());
    SIREN_TEST_ASSERT(s == 1);
    scheduler.resumeFiber(fh);
    scheduler.run();
    SIREN_TEST_ASSERT(!scheduler.hasAliveFibers());
    SIREN_TEST_ASSERT(s == 2);
}

}
