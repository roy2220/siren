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


SIREN_TEST("Check tag alignments")
{
    struct Foo {
        char bar[13];
    };

    static_assert(sizeof(Foo) == 13, "");
    static_assert(alignof(Foo) == 1, "");

    {
        ObjectPool<Foo> op(0, 1, 5);
        Foo *p = op.createObject();
        void *t = op.getObjectTag(p);
        op.destroyObject(p);
        SIREN_TEST_ASSERT((char *)t == (char *)p + sizeof(*p));
    }

    {
        ObjectPool<Foo> op(0, 2, 5);
        Foo *p = op.createObject();
        void *t = op.getObjectTag(p);
        op.destroyObject(p);
        SIREN_TEST_ASSERT((char *)t == (char *)p + sizeof(*p) + 1);
    }

    {
        ObjectPool<Foo> op(0, 3, 5);
        Foo *p = op.createObject();
        void *t = op.getObjectTag(p);
        op.destroyObject(p);
        SIREN_TEST_ASSERT((char *)t == (char *)p + sizeof(*p) + 3);
    }

    {
        ObjectPool<Foo> op(0, 5, 5);
        Foo *p = op.createObject();
        void *t = op.getObjectTag(p);
        op.destroyObject(p);
        SIREN_TEST_ASSERT((char *)t == (char *)p + sizeof(*p) + 3);
    }
}

}
