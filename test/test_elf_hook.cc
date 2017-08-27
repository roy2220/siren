#ifdef SIREN_WITH_VALGRIND

#include <sys/socket.h>
#include <sys/types.h>

#include "elf_hook.h"
#include "test.h"


namespace {

using namespace siren;


int
my_listen(int, int)
{
    return 99;
}


SIREN_TEST("Toggle ELF hooks")
{
    auto h = MakeELFHook(nullptr, "listen", my_listen);
    int ret;
    ret = listen(-1, 0);
    SIREN_TEST_ASSERT(ret < 0);
    h.toggle();
    ret = listen(-1, 0);
    SIREN_TEST_ASSERT(ret == 99);
    h.toggle();
    ret = listen(-1, 0);
    SIREN_TEST_ASSERT(ret < 0);
}

}

#endif
