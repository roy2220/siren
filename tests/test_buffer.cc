#include "buffer.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Get/Set buffers")
{
    Buffer<int> b;
    b.setLength(10);
    SIREN_TEST_ASSERT(b.getLength() >= 10);
    SIREN_TEST_ASSERT(b.get() != nullptr);

    for (int i = 0; i < 10; ++i) {
        b.get()[i] = i;
    }

    b.setLength(666);

    for (int i = 0; i < 10; ++i) {
        SIREN_TEST_ASSERT(b.get()[i] == i);
    }

    b.setLength(0);
    SIREN_TEST_ASSERT(b.get() == nullptr);
}

}
