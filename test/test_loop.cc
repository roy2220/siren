#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include "loop.h"
#include "test.h"

#include "c_library.h"


namespace {

using namespace siren;


SIREN_TEST("Read/Write loop pipe")
{
    siren_fcntl(0, 0, 0);
    int fds[2];
    Loop loop(16 * 1024);
    loop.pipe(fds);
    int dummySize = fcntl(fds[1], F_GETPIPE_SZ, 0);
    void *dummy = std::malloc(dummySize);
    std::memset(dummy, 0, dummySize);

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
