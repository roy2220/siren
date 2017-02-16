#include <cassert>
#include <cstdint>

#include <unistd.h>

#include "helper_macros.h"
#include "test.h"
#include "thread_pool.h"


namespace {

using namespace siren;


SIREN_TEST("Add thread pool tasks")
{
    struct MyThreadPoolTask : ThreadPoolTask
    {
    };

    int a[5];
    ThreadPool tp(3);
    MyThreadPoolTask ts[5];

    for (int i = 0; i < 5; ++i) {
        tp.addTask(&ts[i], [&a, i] () -> void {
            a[i] = i;
            throw i;
        });
    }

    int n = 5;

    do {
        std::uint64_t dummy;
        int r = read(tp.getEventFD(), &dummy, sizeof(dummy));
        SIREN_UNUSED(r);
        assert(r == sizeof(dummy));
        std::vector<ThreadPoolTask *> ts2;
        tp.getCompletedTasks(&ts2);

        for (ThreadPoolTask *t : ts2) {
            int j = static_cast<MyThreadPoolTask *>(t) - ts;
            SIREN_TEST_ASSERT(j >= 0 && j < 5);

            try {
                t->check();
            } catch (int k) {
                SIREN_TEST_ASSERT(k >= 0 && k < 5);
            }
        }

        n -= ts2.size();
    } while (n >= 1);
}


SIREN_TEST("Remove thread pool tasks")
{
    struct MyThreadPoolTask : ThreadPoolTask
    {
    };

    ThreadPool tp(1);
    MyThreadPoolTask t;
    bool done = false;

    tp.addTask(&t, [&] () -> void {
        usleep(100 * 1000);
        done = true;
    });

    bool taskIsCompleted;
    tp.removeTask(&t, &taskIsCompleted);

    if (taskIsCompleted) {
        SIREN_TEST_ASSERT(done);
    } else {
        SIREN_TEST_ASSERT(!done);
    }

    t.check();
}

}
