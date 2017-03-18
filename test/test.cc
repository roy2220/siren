#include <cstdio>
#include <cstdlib>

#include "test.h"


int main()
{
    using namespace siren;

    std::size_t n = GetNumberOfTests();
    std::size_t m = RunTests();
    std::printf("%zu/%zu passed\n", m, n);
    return m < n ? EXIT_FAILURE : EXIT_SUCCESS;
}
