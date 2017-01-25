#pragma once


#include <cstddef>
#include <exception>
#include <string>

#include "helper_macros.h"


#define SIREN_TEST_IMPL SIREN_CONCAT(SirenTestImpl, __LINE__)

#define SIREN_TEST(DESCRIPTION)                                       \
    class SIREN_TEST_IMPL final                                       \
      : public ::siren::detail::Test                                  \
    {                                                                 \
    public:                                                           \
        explicit SIREN_TEST_IMPL() {}                                 \
                                                                      \
        const char *getFileName() const noexcept override {           \
            return __FILE__;                                          \
        }                                                             \
                                                                      \
        unsigned int getLineNumber() const noexcept override {        \
            return __LINE__;                                          \
        }                                                             \
                                                                      \
        const char *getDescription() const noexcept override {        \
            return (DESCRIPTION);                                     \
        }                                                             \
                                                                      \
        void run() override {                                         \
            Run();                                                    \
        }                                                             \
                                                                      \
    private:                                                          \
        static void Run();                                            \
                                                                      \
        SIREN_TEST_IMPL(const SIREN_TEST_IMPL &) = delete;            \
        SIREN_TEST_IMPL &operator=(const SIREN_TEST_IMPL &) = delete; \
    } SIREN_TEST_IMPL;                                                \
                                                                      \
                                                                      \
    void                                                              \
    SIREN_TEST_IMPL::Run()

#define SIREN_TEST_ASSERT(EXPRESSION)                                                     \
    do {                                                                                  \
        if (!(EXPRESSION)) {                                                              \
            throw ::siren::detail::TestAssertionFailure(SIREN_STR(EXPRESSION), __LINE__); \
        }                                                                                 \
    } while (false)


namespace siren {

namespace detail {

class Test
{
public:
    virtual const char *getFileName() const noexcept = 0;
    virtual unsigned int getLineNumber() const noexcept = 0;
    virtual const char *getDescription() const noexcept = 0;
    virtual void run() = 0;

protected:
    inline explicit Test();

    ~Test() = default;

private:
    Test(const Test &) = delete;
    Test &operator=(const Test &) = delete;
};


class TestAssertionFailure final
  : public std::exception
{
public:
    inline explicit TestAssertionFailure(const char *, unsigned int);

    inline const char *what() const noexcept override;

private:
    std::string description_;
};


void AddTest(Test *);

}


std::size_t RunTests() noexcept;

}


/*
 * #include "test-inl.h"
 */


#include <utility>


namespace siren {

namespace detail {

Test::Test()
{
    AddTest(this);
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
    return description_.data();
}

}

}
