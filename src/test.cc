#include "test.h"

#include <cstdio>
#include <exception>
#include <vector>


namespace siren {

namespace {

std::vector<Test_ *> Tests;

}


int RunTests()
{
    int failedTestCount = 0;

    for (Test_ *test : Tests) {
        try {
            test->run();
        } catch (const std::exception &exception) {
            std::fprintf(stderr, "%s:%d: %s: %s\n", test->getFileName()
                         , test->getLineNumber(), test->getDescription(), exception.what());
            ++failedTestCount;
            continue;
        }
    }

    return failedTestCount;
}


void AddTest_(Test_ *test)
{
    Tests.push_back(test);
}

}
