#include <cstdlib>
#include <utility>

#include "rc_pointer.h"
#include "test.h"


namespace {

using namespace siren;


struct Foo
{
    int rc;
    char c[1];
};


void
AddObjectRef(Foo *f) noexcept
{
    ++f->rc;
}


void
ReleaseObject(Foo *f) noexcept
{
    if (--f->rc == 0) {
        std::free(f);
    }
}


SIREN_TEST("Test reference-counting pointer")
{
    Foo *f = reinterpret_cast<Foo *>(std::malloc(100));
    f->rc = 1;
    RCPointer<Foo> p1(f);

    {
        RCPointer<Foo> p2 = p1;
        SIREN_TEST_ASSERT(f->rc == 2);
    }

    RCPointer<Foo> p3 = std::move(p1);
    SIREN_TEST_ASSERT(f->rc == 1);
    p1 = p3;
    SIREN_TEST_ASSERT(f->rc == 2);
    p3.reset();
    SIREN_TEST_ASSERT(f->rc == 1);
}

}
