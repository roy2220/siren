#include "test.h"

#include <cstdio>
#include <exception>
#include <list>


namespace siren {

namespace {

std::list<Test_ *> Tests;

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
        }
    }

    return failedTestCount;
}


void AddTest_(Test_ *test)
{
    Tests.push_back(test);
}

}
