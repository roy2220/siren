#include <cstdlib>

#include "test.h"


int main()
{
    using namespace siren;

    return RunTests() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
