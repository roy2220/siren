#include <utility>

#include "buffer.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Get/Set buffers")
{
    Buffer<int> b;
    b.setLength(10);
    SIREN_TEST_ASSERT(b.getLength() >= 10);
    SIREN_TEST_ASSERT(b != nullptr);

    for (int i = 0; i < 10; ++i) {
        b[i] = i;
    }

    b.setLength(666);

    for (int i = 0; i < 10; ++i) {
        SIREN_TEST_ASSERT(b[i] == i);
    }

    b.setLength(0);
    SIREN_TEST_ASSERT(b == nullptr);
}


SIREN_TEST("Move buffers")
{
    Buffer<int> b1;
    b1.setLength(10);

    for (int i = 0; i < 10; ++i) {
        b1[i] = i;
    }

    Buffer<int> b2 = std::move(b1);
    SIREN_TEST_ASSERT(b1.getLength() == 0);
    SIREN_TEST_ASSERT(b2.getLength() >= 10);

    for (int i = 0; i < 10; ++i) {
        SIREN_TEST_ASSERT(b2[i] == i);
    }

    b1 = std::move(b2);
    SIREN_TEST_ASSERT(b1.getLength() >= 10);
    SIREN_TEST_ASSERT(b2.getLength() == 0);

    for (int i = 0; i < 10; ++i) {
        SIREN_TEST_ASSERT(b1[i] == i);
    }
}

}
