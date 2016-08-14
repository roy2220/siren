#include <list>

#include "semaphore.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Up/Down semaphores")
{
    Scheduler sched;
    Semaphore sem(&sched, 0, 10, 0);
    std::list<int> p;

    sched.createFiber([&sem, &p] () -> void {
        for (int i = 0; i < 100; ++i) {
            p.push_back(i);
            SIREN_TEST_ASSERT(p.size() <= 11);
            sem.up();
        }
    });

    sched.createFiber([&sem, &p] () -> void {
        for (int i = 0; i < 100; ++i) {
            sem.down();
            SIREN_TEST_ASSERT(!p.empty());
            p.pop_front();
        }
    });

    sched.run();
}


SIREN_TEST("Try up/down semaphores")
{
    Scheduler sched;
    Semaphore sem(&sched, -1, 2, 1);
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

