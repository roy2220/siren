#pragma once


#include <exception>
#include <string>

#include "helper_macros.h"


#define SIREN_TEST_IMPL CONCAT(SirenTestImpl, __LINE__)

#define SIREN_TEST(DESCRIPTION)                                       \
    class SIREN_TEST_IMPL final                                       \
      : public ::siren::detail::Test                                  \
    {                                                                 \
    public:                                                           \
        explicit SIREN_TEST_IMPL();                                   \
                                                                      \
        const char *getFileName() const noexcept override;            \
        int getLineNumber() const noexcept override;                  \
        const char *getDescription() const noexcept override;         \
        void run() const override;                                    \
                                                                      \
    private:                                                          \
        SIREN_TEST_IMPL(const SIREN_TEST_IMPL &) = delete;            \
        SIREN_TEST_IMPL &operator=(const SIREN_TEST_IMPL &) = delete; \
    } SIREN_TEST_IMPL;                                                \
                                                                      \
                                                                      \
    SIREN_TEST_IMPL::SIREN_TEST_IMPL()                                \
    {                                                                 \
    }                                                                 \
                                                                      \
                                                                      \
    const char *                                                      \
    SIREN_TEST_IMPL::getFileName() const noexcept                     \
    {                                                                 \
        return __FILE__;                                              \
    }                                                                 \
                                                                      \
                                                                      \
    int                                                               \
    SIREN_TEST_IMPL::getLineNumber() const noexcept                   \
    {                                                                 \
        return __LINE__;                                              \
    }                                                                 \
                                                                      \
                                                                      \
    const char *                                                      \
    SIREN_TEST_IMPL::getDescription() const noexcept                  \
    {                                                                 \
        return (DESCRIPTION);                                         \
    }                                                                 \
                                                                      \
                                                                      \
    void                                                              \
    SIREN_TEST_IMPL::run() const

#define SIREN_TEST_ASSERT(EXPRESSION)                                               \
    do {                                                                            \
        if (!(EXPRESSION)) {                                                        \
            throw ::siren::detail::TestAssertionFailure(STR(EXPRESSION), __LINE__); \
        }                                                                           \
    } while (false)


namespace siren {

namespace detail {

class Test
{
public:
    virtual const char *getFileName() const noexcept = 0;
    virtual int getLineNumber() const noexcept = 0;
    virtual const char *getDescription() const noexcept = 0;
    virtual void run() const = 0;

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
    inline explicit TestAssertionFailure(const char *, int);

    inline const char *what() const noexcept override;

    TestAssertionFailure(TestAssertionFailure &&) = default;
    TestAssertionFailure &operator=(TestAssertionFailure &&) = default;

private:
    std::string description_;

    TestAssertionFailure(const TestAssertionFailure &) = delete;
    TestAssertionFailure &operator=(const TestAssertionFailure &) = delete;
};


void AddTest(Test *);

}


int RunTests();

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


TestAssertionFailure::TestAssertionFailure(const char *expression, int lineNumber)
{
    description_ = "SIREN_TEST_ASSERT(";
    description_ += expression;
    description_ += ") failed at line ";
    description_ += std::to_string(lineNumber);
}


const char *
TestAssertionFailure::what() const noexcept
{
    return description_.data();
}

}

}
