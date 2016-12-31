#include <list>

#include "semaphore.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Up/Down semaphores")
{
    Scheduler sched;
    Semaphore sem(&sched, 0, 0, 10);
    std::list<int> p;

    sched.createFiber([&sem, &p] () -> void {
        for (int i = 0; i < 100; ++i) {
            p.push_back(i);
            SIREN_TEST_ASSERT(p.size() <= 11);
            sem.up();
        }
    });

    void *fh = sched.createFiber([&sem, &p] () -> void {
        for (int i = 0; i < 100; ++i) {
            sem.down();
            SIREN_TEST_ASSERT(!p.empty());
            p.pop_front();
        }

        try {
            sem.down();
        } catch (const FiberInterruption &) {
            throw 2349;
        }
    });

    sched.run();
    int i = 0;

    try {
        sched.interruptFiber(fh);
    } catch (int x) {
        i = x;
    }

    SIREN_TEST_ASSERT(i == 2349);
}


SIREN_TEST("Try up/down semaphores")
{
    Scheduler sched;
    Semaphore sem(&sched, 1, -1, 2);
    SIREN_TEST_ASSERT(sem.tryDown());
    SIREN_TEST_ASSERT(sem.tryUp());
    SIREN_TEST_ASSERT(sem.tryDown());
    SIREN_TEST_ASSERT(sem.tryUp());
    SIREN_TEST_ASSERT(sem.tryUp());
    SIREN_TEST_ASSERT(!sem.tryUp());
    SIREN_TEST_ASSERT(sem.tryDown());
    SIREN_TEST_ASSERT(sem.tryDown());
    SIREN_TEST_ASSERT(sem.tryDown());
    SIREN_TEST_ASSERT(!sem.tryDown());
}

}

