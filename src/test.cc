#include "test.h"

#include <exception>
#include <iostream>
#include <vector>


namespace siren {

namespace {

std::vector<Test *> Tests;

}


int RunTests()
{
    int failedTestCount = 0;

    for (Test *test : Tests) {
        try {
            test->run();
        } catch (const std::exception &exception) {
            std::cerr << test->getFileName() << ':' << test->getLineNumber() << ": "
            << test->getDescription() << " failed: " << exception.what() << std::endl;
            ++failedTestCount;
        }
    }

    return failedTestCount;
}


void _AddTest(Test *test)
{
    Tests.push_back(test);
}

}
