#include <random>

#include "mutex.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Lock/Unlock mutexes")
{
    Scheduler sched;
    Mutex m(&sched);
    std::mt19937 gen;
    char c[2];

    for (int i = 0; i < 26; ++i) {
        sched.createFiber([&sched, &m, &gen, i, &c] () -> void {
            m.lock();
            bool b = gen() % 2;
            c[b] = 'A' + i;
            sched.yieldTo();
            c[!b] = 'A' + i;
            m.unlock();
        });
    }

    sched.run();
    SIREN_TEST_ASSERT(c[0] == c[1]);
}

}

