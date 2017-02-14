#include "async.h"
#include "ip_endpoint.h"
#include "loop.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Make ip endpoints")
{
    Loop l;
    Async a(&l, 1);

    l.createFiber([&] () -> void {
        IPEndpoint ipe = a.makeIPEndpoint("localhost", "http");
        SIREN_TEST_ASSERT(ipe.address == ((127 << 24) | 1));
        SIREN_TEST_ASSERT(ipe.portNumber == 80);
    });

    l.run();
}

}
