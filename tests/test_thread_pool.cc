#include <cassert>

#include <unistd.h>

#include "helper_macros.h"
#include "test.h"
#include "thread_pool.h"


namespace {

using namespace siren;


SIREN_TEST("Post thread pool works")
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

    for (int i = 0; i < 5; ++i) {
        MyThreadPoolTask *t;
        int r = read(tp.getFD(), &t, sizeof(t));
        SIREN_UNUSED(r);
        assert(r == sizeof(t));
        int j = t - ts;
        SIREN_TEST_ASSERT(j >= 0 && j < 5);

        try {
            t->finish();
        } catch (int k) {
            SIREN_TEST_ASSERT(k >= 0 && k < 5);
        }
    }
}

}
