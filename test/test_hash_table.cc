#include "hash_table.h"
#include "test.h"

#include <algorithm>
#include <functional>
#include <random>


namespace {

using namespace siren;


SIREN_TEST("Search hash table")
{
    struct Dummy : HashTableNode {
        int val;

        static bool OrderRBTreeNode(const HashTableNode *x, const HashTableNode *y) {
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
    HashTable ht;

    for (int i : a) {
        d[i].val = i;
        ht.insertNode(&d[i], std::hash<int>()(i));
    }

    for (int i = 0; i < 512; ++i) {
        ht.removeNode(&d[i]);
    }

    ht.traverse([&] (HashTableNode *n) -> void {
        auto pd = static_cast<Dummy *>(n);
        SIREN_TEST_ASSERT(pd - d >= 512);
    });

    for (int i = 0; i < 512; ++i) {
        ht.insertNode(&d[i], std::hash<int>()(i));
    }

    for (int i : a) {
        HashTableNode *n = ht.search(std::hash<int>()(i), [i] (HashTableNode *n) -> bool {
            auto pd = static_cast<Dummy *>(n);
            return i == pd->val;
        });

        SIREN_TEST_ASSERT(n == &d[i]);
        ht.removeNode(n);
    }

    SIREN_TEST_ASSERT(ht.isEmpty());
}

}
