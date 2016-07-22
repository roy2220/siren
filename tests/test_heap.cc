#include "heap.h"
#include "test.h"

#include <random>


namespace {

using namespace siren;


SIREN_TEST("Insert/Remove heap nodes")
{
    struct Dummy : Heap::Node {
        int val;

        static bool OrderHeapNode(const Heap::Node &hn1, const Heap::Node &hn2) {
            return static_cast<const Dummy &>(hn1).val <=
                   static_cast<const Dummy &>(hn2).val;
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

    auto pd = static_cast<Dummy *>(h.getTop());
    SIREN_TEST_ASSERT(pd != nullptr);
    h.removeTop();
    int t = pd->val;

    for (;;) {
        pd = static_cast<Dummy *>(h.getTop());

        if (pd == nullptr) {
            break;
        }

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
    struct Dummy : Heap::Node {
        int val;

        Dummy(int x) : val(x) {}

        static bool OrderHeapNode(const Heap::Node &hn1, const Heap::Node &hn2) {
            return static_cast<const Dummy &>(hn1).val <=
                   static_cast<const Dummy &>(hn2).val;
        }
    };

    Heap h1(Dummy::OrderHeapNode);
    Dummy d[] = {23, 48, -1, -7777773, 999};

    for (int i = 0; i < 5; ++i) {
        h1.insertNode(&d[i]);
    }

    Heap h2 = std::move(h1);
    SIREN_TEST_ASSERT(h1.getTop() == nullptr);

    for (int i = 0; i < 5; ++i) {
        SIREN_TEST_ASSERT(h2.getTop() != nullptr);
        h2.removeTop();
    }

    for (int i = 0; i < 5; ++i) {
        h2.insertNode(&d[i]);
    }

    h1 = std::move(h2);
    SIREN_TEST_ASSERT(h2.getTop() == nullptr);

    for (int i = 0; i < 5; ++i) {
        SIREN_TEST_ASSERT(h1.getTop() != nullptr);
        h1.removeTop();
    }
}

}
