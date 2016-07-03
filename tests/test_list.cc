#include "list.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Insert/Replace/Remove list items")
{
    struct Dummy : List::Node {
    };

    List l;
    SIREN_TEST_ASSERT(l.isEmpty());
    Dummy d1;
    Dummy d2;
    l.insertHead(&d1);
    l.insertTail(&d2);
    SIREN_TEST_ASSERT(l.getHead() == &d1);
    SIREN_TEST_ASSERT(l.getTail() == &d2);
    d1.remove();
    SIREN_TEST_ASSERT(l.getHead() == &d2);
    d1.insertAfter(&d2);
    d2.remove();
    SIREN_TEST_ASSERT(l.getTail() == &d1);
    d2.insertBefore(&d1);
    d1.remove();
    d2.replace(&d1);
    SIREN_TEST_ASSERT(l.getTail() == &d1);
    d1.remove();
    SIREN_TEST_ASSERT(l.isEmpty());
}


SIREN_TEST("Iterate lists")
{
    struct Dummy : List::Node {
        int val;

        Dummy(int x) : val(x) {
        }
    };

    int val = 0;
    Dummy d1(val++);
    Dummy d2(val++);
    Dummy d3(val++);
    List l;
    l.insertTail(&d1);
    l.insertTail(&d2);
    l.insertTail(&d3);

    SIREN_LIST_FOR_EACH_NODE_REVERSE(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == --val);
    }

    SIREN_LIST_FOR_EACH_NODE(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == val++);
    }
}


SIREN_TEST("Iterate lists while removing items")
{
    struct Dummy : List::Node {
        int val;

        Dummy(int x) : val(x) {
        }
    };

    int val = 0;
    Dummy d1(val++);
    Dummy d2(val++);
    Dummy d3(val++);
    List l;
    l.insertTail(&d1);
    l.insertTail(&d2);
    l.insertTail(&d3);

    SIREN_LIST_FOR_EACH_NODE_SAFE_REVERSE(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == --val);
        d->remove();
    }

    SIREN_TEST_ASSERT(l.isEmpty());
    l.insertTail(&d1);
    l.insertTail(&d2);
    l.insertTail(&d3);

    SIREN_LIST_FOR_EACH_NODE_SAFE(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == val++);
        d->remove();
    }

    SIREN_TEST_ASSERT(l.isEmpty());
}

}
