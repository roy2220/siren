#include "rb_tree.h"
#include "test.h"

#include <algorithm>
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

    std::mt19937 gen((std::random_device())());
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
        auto pd = static_cast<Dummy *>(n);
        unsigned int v = pd->val;
        SIREN_TEST_ASSERT(v >= pv);
        SIREN_TEST_ASSERT(pd - d >= 512);
        pv = v;
    });

    pv = 0;

    for (RBTreeNode *n = rbt.getRoot(); !rbt.isNil(n); n = rbt.findNodeNext(n)) {
        auto pd = static_cast<Dummy *>(n);
        unsigned int v = pd->val;
        SIREN_TEST_ASSERT(v >= pv);
        SIREN_TEST_ASSERT(pd - d >= 512);
        pv = v;
    }

    pv = -1;

    for (RBTreeNode *n = rbt.getRoot(); !rbt.isNil(n); n = rbt.findNodePrev(n)) {
        auto pd = static_cast<Dummy *>(n);
        unsigned int v = pd->val;
        SIREN_TEST_ASSERT(v <= pv);
        SIREN_TEST_ASSERT(pd - d >= 512);
        pv = v;
    }

    for (int i = 512; i < 1024; ++i) {
        rbt.removeNode(&d[i]);
    }

    SIREN_TEST_ASSERT(rbt.isEmpty());
}


SIREN_TEST("Search red-black tree")
{
    struct Dummy : RBTreeNode {
        int val;

        static bool OrderRBTreeNode(const RBTreeNode *x, const RBTreeNode *y) {
            int v1 = static_cast<const Dummy *>(x)->val;
            int v2 = static_cast<const Dummy *>(y)->val;
            return v1 <= v2;
        }
    };

    std::mt19937 gen((std::random_device())());
    Dummy d[1024];
    int a[1024];

    for (int i = 0; i < 1024; ++i) {
        a[i] = i;
    }

    std::shuffle(&a[0], &a[1023], gen);
    RBTree rbt(Dummy::OrderRBTreeNode);

    for (int i : a) {
        d[i].val = i;
        rbt.insertNode(&d[i]);
    }

    for (int i : a) {
        RBTreeNode *n = rbt.search([i] (RBTreeNode *n) -> int {
            auto pd = static_cast<Dummy *>(n);
            return i - pd->val;
        });

        SIREN_TEST_ASSERT(n == &d[i]);
        rbt.removeNode(n);
    }

    SIREN_TEST_ASSERT(rbt.isEmpty());
}


SIREN_TEST("Move red-black trees")
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
