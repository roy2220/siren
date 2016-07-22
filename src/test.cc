#include "test.h"

#include <cstdio>
#include <exception>
#include <list>


namespace siren {

namespace {

std::list<detail::Test *> Tests;

}


namespace detail {

void
AddTest(Test *test)
{
    Tests.push_back(test);
}

}


std::size_t
RunTests() noexcept
{
    std::size_t failedTestCount = 0;

    for (detail::Test *test : Tests) {
        try {
            test->run();
        } catch (const std::exception &exception) {
            std::fprintf(stderr, "%s:%d: %s: %s\n", test->getFileName()
                         , test->getLineNumber(), test->getDescription(), exception.what());
            ++failedTestCount;
        }
    }

    return failedTestCount;
}

}
