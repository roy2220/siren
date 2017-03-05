#include <cstdlib>
#include <utility>

#include "rc_pointer.h"
#include "test.h"


namespace {

using namespace siren;


struct Foo
  : RCRecord
{
    int *p;
    Foo(int *p) : p(p) {}
    ~Foo() { *p = -99; }
};


SIREN_TEST("Test reference-counting pointer")
{
    int i = 0;
    RCPointer<Foo> p1(new Foo{&i});

    {
        RCPointer<Foo> p2 = p1;
    }

    RCPointer<Foo> p3 = std::move(p1);
    p1 = p3;
    p3.reset();
    p3 = p1.get();
    p1 = nullptr;
    SIREN_TEST_ASSERT(i == 0);
    p3 = nullptr;
    SIREN_TEST_ASSERT(i == -99);
}

}


namespace siren {

namespace detail {

template <>
void
DestroyObject(Foo *f) noexcept
{
    delete f;
}

}

}
