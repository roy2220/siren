#include "test.h"

#include <cstdio>
#include <exception>
#include <list>
#include <utility>


namespace siren {

namespace {

std::list<detail::Test *> &Tests();

} // namespace


std::size_t
GetNumberOfTests() noexcept
{
    return Tests().size();
}


std::size_t
RunTests() noexcept
{
    std::size_t passedTestCount = 0;

    for (detail::Test *test : Tests()) {
        try {
            test->run();
            ++passedTestCount;
        } catch (const std::exception &exception) {
            std::fprintf(stderr, "%s:%u: %s: %s\n", test->getFileName()
                         , test->getLineNumber(), test->getDescription(), exception.what());
        }
    }

    return passedTestCount;
}


namespace detail {

Test::Test()
{
    Tests().push_back(this);
}


TestAssertionFailure::TestAssertionFailure(const char *expression, unsigned int lineNumber)
{
    description_ = "Assert `";
    description_ += expression;
    description_ += "` failed at line ";
    description_ += std::to_string(lineNumber);
}


const char *
TestAssertionFailure::what() const noexcept
{
    return description_.c_str();
}

} // namespace detail


namespace {

std::list<detail::Test *> &
Tests()
{
    static std::list<detail::Test *> tests;
    return tests;
}


} // namespace

} // namespace siren
