#include "rb_tree.h"
#include "test.h"

#include <random>
#include <utility>


namespace {

using namespace siren;


SIREN_TEST("Insert/Remove red-black tree nodes")
{
    struct Dummy : RBTreeNode {
        unsigned int val;

        static bool OrderRBTreeNode(const RBTreeNode *x, const RBTreeNode *y) {
            unsigned int v1 = static_cast<const Dummy *>(x)->val;
            unsigned int v2 = static_cast<const Dummy *>(y)->val;
            return v1 <= v2;
        }
    };


    std::mt19937 gen;
    gen.seed(1);
    Dummy d[1024];
    RBTree rbt(Dummy::OrderRBTreeNode);

    for (int i = 0; i < 1024; ++i) {
        d[i].val = gen();
        rbt.insertNode(&d[i]);
    }

    for (int i = 0; i < 512; ++i) {
        rbt.removeNode(&d[i]);
    }

    unsigned int pv = 0;

    rbt.traverse([&] (RBTreeNode *n) -> void {
        unsigned int v = static_cast<Dummy *>(n)->val;
        SIREN_TEST_ASSERT(v >= pv);
        pv = v;
    });

    for (int i = 512; i < 1024; ++i) {
        rbt.removeNode(&d[i]);
    }

    SIREN_TEST_ASSERT(rbt.isEmpty());
}


SIREN_TEST("Move red-black tree")
{
    struct Dummy : RBTreeNode {
        unsigned int val;

        static bool OrderRBTreeNode(const RBTreeNode *x, const RBTreeNode *y) {
            unsigned int v1 = static_cast<const Dummy *>(x)->val;
            unsigned int v2 = static_cast<const Dummy *>(y)->val;
            return v1 <= v2;
        }
    };

    RBTree rbt1(Dummy::OrderRBTreeNode);
    Dummy d;
    d.val = 0;

    {
        RBTree rbt2(Dummy::OrderRBTreeNode);
        rbt2.insertNode(&d);
        SIREN_TEST_ASSERT(rbt2.isNil(d.getLeftChild()));
        SIREN_TEST_ASSERT(rbt2.isNil(d.getRightChild()));
        rbt1 = std::move(rbt2);
        SIREN_TEST_ASSERT(!rbt2.isNil(d.getLeftChild()));
        SIREN_TEST_ASSERT(!rbt2.isNil(d.getRightChild()));
    }

    SIREN_TEST_ASSERT(rbt1.isNil(d.getLeftChild()));
    SIREN_TEST_ASSERT(rbt1.isNil(d.getRightChild()));
}

}
