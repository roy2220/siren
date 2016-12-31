#include "loop.h"
#include "test.h"

#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>


namespace {

using namespace siren;


SIREN_TEST("Read/Write loop pipe")
{
    int fds[2];
    Loop loop(16 * 1024);
    loop.pipe(fds);
    int dummySize = fcntl(fds[1], F_GETPIPE_SZ, 0);
    void *dummy = std::malloc(dummySize);

    loop.createFiber([&] () -> void {
        while (loop.read(fds[0], dummy, dummySize) >= 1) {}
        loop.close(fds[0]);
    });

    loop.createFiber([&] () -> void {
        for (int i = 0; i < 10; ++i) {
            loop.write(fds[1], dummy, dummySize);
        }

        loop.close(fds[1]);
    });

    loop.run();
    std::free(dummy);
}

}
