#include <cerrno>
#include <functional>

#include "async.h"
#include "loop.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Execute async task")
{
    Loop loop;
    Async async(&loop, 1);

    loop.createFiber([&] () {
        int f = false;

        try {
            async.executeTask([&] () {
                throw 239;
            });
        } catch (int s) {
            f = true;
            SIREN_TEST_ASSERT(s == 239);
        }

        SIREN_TEST_ASSERT(f);
        int t = 100;

        async.executeTask([&] () {
            --t;
        });

        SIREN_TEST_ASSERT(t == 99);
    });

    loop.run();
    int x = 0;

    void *f = loop.createFiber([&] () {
        try {
            async.executeTask([&] () {
                usleep(100 * 1000);
            });

            x = 1;
        } catch (FiberInterruption) {
            x = 2;
        }
    });

    loop.createFiber([&] () {
        loop.interruptFiber(f);
        SIREN_TEST_ASSERT(x == 1 || x == 2);
    });

    loop.run();
}

}


SIREN_TEST("Do async calls")
{
    Loop loop;
    Async async(&loop, 1);

    loop.createFiber([&] () {
        int a = async.callFunction([] (int x, int y) -> int { return x > y ? x : y; }, 43, 99);
        SIREN_TEST_ASSERT(a == 99);
    });

    loop.createFiber([&] () {
        int a = 0;
        async.callFunction([] (int &a) -> void { a = 4399; }, std::ref(a));
        SIREN_TEST_ASSERT(a == 4399);
    });

    loop.createFiber([&] () {
        async.callFunction([] () -> void { errno = 4399; });
        SIREN_TEST_ASSERT(errno == 4399);
    });

    loop.run();
}
