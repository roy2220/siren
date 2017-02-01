#include <random>
#include <utility>

#include "list.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Add/Remove list nodes")
{
    struct Dummy : ListNode {
    };

    List l;
    SIREN_TEST_ASSERT(l.isEmpty());
    Dummy d1;
    Dummy d2;
    l.prependNode(&d1);
    l.appendNode(&d2);
    SIREN_TEST_ASSERT(l.getHead() == &d1);
    SIREN_TEST_ASSERT(l.getTail() == &d2);
    d1.remove();
    SIREN_TEST_ASSERT(l.getHead() == &d2);
    d1.insertAfter(&d2);
    d2.remove();
    SIREN_TEST_ASSERT(l.getTail() == &d1);
    d2.insertBefore(&d1);
    d1.remove();
    SIREN_TEST_ASSERT(l.getHead() == &d2);
    d2.remove();
    SIREN_TEST_ASSERT(l.isEmpty());
}


SIREN_TEST("Iterate lists")
{
    struct Dummy : ListNode {
        int val;

        Dummy(int x) : val(x) {
        }
    };

    int val = 0;
    Dummy d1(val++);
    Dummy d2(val++);
    Dummy d3(val++);
    List l;
    l.appendNode(&d1);
    l.appendNode(&d2);
    l.appendNode(&d3);

    SIREN_LIST_FOREACH_REVERSE(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == --val);
    }

    SIREN_LIST_FOREACH(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == val++);
    }
}


SIREN_TEST("Iterate lists while removing nodes")
{
    struct Dummy : ListNode {
        int val;

        Dummy(int x) : val(x) {
        }
    };

    int val = 0;
    Dummy d1(val++);
    Dummy d2(val++);
    Dummy d3(val++);
    List l;
    l.appendNode(&d1);
    l.appendNode(&d2);
    l.appendNode(&d3);

    SIREN_LIST_FOREACH_SAFE_REVERSE(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == --val);
        d->remove();
    }

    SIREN_TEST_ASSERT(l.isEmpty());
    l.appendNode(&d1);
    l.appendNode(&d2);
    l.appendNode(&d3);

    SIREN_LIST_FOREACH_SAFE(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == val++);
        d->remove();
    }

    SIREN_TEST_ASSERT(l.isEmpty());
}


SIREN_TEST("Move lists")
{
    struct Dummy : ListNode {
        int val;

        Dummy(int x) : val(x) {
        }
    };

    int val = 0;
    Dummy d1(val++);
    Dummy d2(val++);
    Dummy d3(val++);
    Dummy d4(val++);
    List l;
    l.appendNode(&d1);
    l.appendNode(&d2);
    l.appendNode(&d3);

    List l2 = std::move(l);
    SIREN_TEST_ASSERT(l.isEmpty());
    SIREN_TEST_ASSERT(!l2.isEmpty());
    val = 0;

    SIREN_LIST_FOREACH(ln, l2) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == val++);
    }

    l.appendNode(&d4);
    l = std::move(l2);
    l.appendNode(&d4);
    val = 4;

    SIREN_LIST_FOREACH_REVERSE(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == --val);
    }
}


SIREN_TEST("Append/Prepend lists")
{
    struct Dummy : ListNode {
        int val;

        Dummy(int x) : val(x) {
        }
    };

    int val = 0;
    Dummy d1(val++);
    Dummy d2(val++);
    Dummy d3(val++);
    Dummy d4(val++);
    List l1;
    l1.appendNode(&d1);
    l1.appendNode(&d2);
    List l2;
    l2.appendNode(&d3);
    l2.appendNode(&d4);
    SIREN_TEST_ASSERT(!l2.isEmpty());
    l2.append(&l1);
    SIREN_TEST_ASSERT(l2.isEmpty());

    SIREN_LIST_FOREACH_REVERSE(ln, l1) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == --val);
    }

    l2.appendNode((d3.remove(), &d3));
    l2.appendNode((d4.remove(), &d4));
    SIREN_TEST_ASSERT(!l1.isEmpty());
    l1.prepend(&l2);
    SIREN_TEST_ASSERT(l1.isEmpty());

    SIREN_LIST_FOREACH(ln, l2) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val == val++);
    }
}


SIREN_TEST("Sort lists")
{
    std::mt19937 gen;
    gen.seed(19937);

    struct Dummy : ListNode {
        unsigned int val;
    };

    Dummy ds[1024];
    List l;

    for (int i = 0; i < 1024; ++i) {
        ds[i].val = gen();
        l.appendNode(&ds[i]);
    }

    l.sort([] (const ListNode *ln1, const ListNode *ln2) -> bool {
        auto d1 = static_cast<const Dummy *>(ln1);
        auto d2 = static_cast<const Dummy *>(ln2);
        return d1->val <= d2->val;
    });

    unsigned int pval = 0;

    SIREN_LIST_FOREACH(ln, l) {
        auto d = static_cast<Dummy *>(ln);
        SIREN_TEST_ASSERT(d->val >= pval);
        pval = d->val;
    }
}

}
