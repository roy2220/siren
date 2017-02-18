#include <cstdlib>
#include <utility>

#include "rc_pointer.h"
#include "test.h"


namespace siren {

struct Foo
  : RCRecord
{
    char c[100];
};


namespace detail {

template <>
void
DestroyObject(Foo *f) noexcept
{
    delete f;
}

}

}


namespace {

using namespace siren;


SIREN_TEST("Test reference-counting pointer")
{
    RCPointer<Foo> p1(new Foo());

    {
        RCPointer<Foo> p2 = p1;
    }

    RCPointer<Foo> p3 = std::move(p1);
    p1 = p3;
    p3.reset();
    p3 = p1.get();
    p1 = nullptr;
    p3 = nullptr;
}

}
