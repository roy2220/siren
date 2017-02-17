#include "object_pool.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Create/Destroy objects")
{
    struct Foo {
        int *p;

        Foo (int *p) : p(p) { *p = -1; }
        ~Foo () { *p = 100; }
    };

    ObjectPool<Foo> op(1);
    int x;
    Foo *f = op.createObject(&x);
    SIREN_TEST_ASSERT(x == -1);
    op.destroyObject(f);
    SIREN_TEST_ASSERT(x == 100);
}

}
