#include "heap.h"
#include "test.h"

#include <random>
#include <utility>


namespace {

using namespace siren;


SIREN_TEST("Insert/Remove heap nodes")
{
    struct Dummy : HeapNode {
        int val;

        static bool OrderHeapNode(const HeapNode *hn1, const HeapNode *hn2) {
            return static_cast<const Dummy *>(hn1)->val <=
                   static_cast<const Dummy *>(hn2)->val;
        }
    };

    std::mt19937 gen;
    gen.seed(1);
    Dummy d[1024];
    Heap h(Dummy::OrderHeapNode);

    for (int i = 0; i < 1024; ++i) {
        d[i].val = gen();
        h.insertNode(&d[i]);
    }

    for (int i = 0; i < 512; ++i) {
        h.removeNode(&d[i]);
    }

    SIREN_TEST_ASSERT(!h.isEmpty());
    auto pd = static_cast<Dummy *>(h.getTop());
    h.removeTop();
    int t = pd->val;

    while (!h.isEmpty()) {
        pd = static_cast<Dummy *>(h.getTop());
        h.removeTop();
        SIREN_TEST_ASSERT(pd->val >= t);
        t = pd->val;
    }

    for (int i = 0; i < 1024; ++i) {
        h.insertNode(&d[i]);
    }
}


SIREN_TEST("Move heap nodes")
{
    struct Dummy : HeapNode {
        int val;

        Dummy(int x) : val(x) {}
        Dummy(Dummy &&other) : val(other.val) {}

        static bool OrderHeapNode(const HeapNode *hn1, const HeapNode *hn2) {
            return static_cast<const Dummy *>(hn1)->val <=
                   static_cast<const Dummy *>(hn2)->val;
        }
    };

    Heap h1(Dummy::OrderHeapNode);
    Dummy d[] = {23, 48, -1, -7777773, 999};

    for (int i = 0; i < 5; ++i) {
        h1.insertNode(&d[i]);
    }

    Heap h2 = std::move(h1);
    SIREN_TEST_ASSERT(h1.isEmpty());

    for (int i = 0; i < 5; ++i) {
        SIREN_TEST_ASSERT(!h2.isEmpty());
        h2.removeTop();
    }

    for (int i = 0; i < 5; ++i) {
        h2.insertNode(&d[i]);
    }

    h1 = std::move(h2);
    SIREN_TEST_ASSERT(h2.isEmpty());

    for (int i = 0; i < 5; ++i) {
        SIREN_TEST_ASSERT(!h1.isEmpty());
        h1.removeTop();
    }
}

}
