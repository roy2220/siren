#include <utility>

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

        SIREN_TEST_ASSERT(scheduler.getNumberOfAliveFibers() == 2) ;
        SIREN_TEST_ASSERT(scheduler.getNumberOfActiveFibers() == 0) ;
        scheduler.run();
        SIREN_TEST_ASSERT(scheduler.getNumberOfAliveFibers() == 2) ;
        SIREN_TEST_ASSERT(scheduler.getNumberOfActiveFibers() == 2) ;
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
    SIREN_TEST_ASSERT(scheduler.getNumberOfAliveFibers() == 0);
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

    SIREN_TEST_ASSERT(scheduler.getNumberOfAliveFibers() == 1);
    scheduler.suspendFiber(fh);
    SIREN_TEST_ASSERT(scheduler.getNumberOfActiveFibers() == 0);
    scheduler.run();
    SIREN_TEST_ASSERT(scheduler.getNumberOfActiveFibers() == 0);
    SIREN_TEST_ASSERT(s == 0);
    scheduler.resumeFiber(fh);
    scheduler.run();
    SIREN_TEST_ASSERT(scheduler.getNumberOfActiveFibers() == 1);
    SIREN_TEST_ASSERT(s == 1);
    scheduler.resumeFiber(fh);
    scheduler.run();
    SIREN_TEST_ASSERT(scheduler.getNumberOfActiveFibers() == 0);
    SIREN_TEST_ASSERT(scheduler.getNumberOfAliveFibers() == 0);
    SIREN_TEST_ASSERT(s == 2);
}


SIREN_TEST("Create background fibers")
{
    int s = 1;

    {
        Scheduler sched;

        {
            Scheduler temp;
            temp.createFiber([] () -> void {});

            temp.createFiber([&sched, &s] () -> void {
                ++s;
                auto sg = MakeScopeGuard([&s] () -> void { --s; });
                sched.suspendFiber(sched.getCurrentFiber());
            }, 0, true);

            sched = std::move(temp);
        }

        SIREN_TEST_ASSERT(sched.getNumberOfAliveFibers() == 2);
        SIREN_TEST_ASSERT(sched.getNumberOfForegroundFibers() == 1);
        SIREN_TEST_ASSERT(sched.getNumberOfActiveFibers() == 0);
        sched.run();
        SIREN_TEST_ASSERT(sched.getNumberOfAliveFibers() == 1);
        SIREN_TEST_ASSERT(sched.getNumberOfForegroundFibers() == 0);
        SIREN_TEST_ASSERT(sched.getNumberOfActiveFibers() == 1);
        SIREN_TEST_ASSERT(s == 2);
    }

    SIREN_TEST_ASSERT(s == 1);
}

}
