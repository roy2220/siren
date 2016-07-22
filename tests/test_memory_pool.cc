#include <cstddef>
#include <cstdint>

#include "memory_pool.h"
#include "test.h"


namespace {

using namespace siren;


SIREN_TEST("Allocate/Free memory blocks")
{
    struct Dummy {
        unsigned char uc;
        int i;
        bool b;
    };

    MemoryPool mp(alignof(Dummy), sizeof(Dummy), 1);
    void *ps[1024];

    for (int i = 0; i < 1024; ++i) {
        auto d = static_cast<Dummy *>(mp.allocateBlock());
        SIREN_TEST_ASSERT(d != nullptr);
        d->b = i;
        d->uc = i;
        d->i = i;
        ps[i] = d;
    }

    for (int i = 0; i < 1024; ++i) {
        mp.freeBlock(ps[i]);
    }
}


SIREN_TEST("Check memory block pointer alignments")
{
    for (std::size_t n = 1; n < alignof(std::max_align_t); ++n) {
        MemoryPool mp(n, n, 0);
        void *ps[32];

        for (int i = 0; i < 32; ++i) {
            auto p = static_cast<char *>(mp.allocateBlock());
            SIREN_TEST_ASSERT(p != nullptr);
            SIREN_TEST_ASSERT(reinterpret_cast<std::uintptr_t>(p) % alignof(void *) == 0);
            ps[i] = p;
        }

        for (int i = 0; i < 32; ++i) {
            mp.freeBlock(ps[i]);
        }
    }
}


SIREN_TEST("Move memory pools")
{
    MemoryPool mp(0, 0, 0);
    void *p1 = mp.allocateBlock();
    void *p2 = mp.allocateBlock();
    void *p3 = mp.allocateBlock();

    {
        MemoryPool mp2 = std::move(mp);
        mp2.freeBlock(p1);
        mp2.freeBlock(p2);
        mp2.freeBlock(p3);
    }

    p1 = mp.allocateBlock();
    p2 = mp.allocateBlock();
    p3 = mp.allocateBlock();

    {
        MemoryPool mp2(0, 0, 0);
        mp2 = std::move(mp);
        mp2.freeBlock(p1);
        mp2.freeBlock(p2);
        mp2.freeBlock(p3);
    }
}

}
