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
            return static_cast<const Dummy &>(hn1).val <
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
    h.removeNode(pd);
    int t = pd->val;

    for (int i = 1; i < 512; ++i) {
        pd = static_cast<Dummy *>(h.getTop());
        h.removeNode(pd);
        SIREN_TEST_ASSERT(pd->val >= t);
        t = pd->val;
    }

    SIREN_TEST_ASSERT(h.getTop() == nullptr);
}

}
