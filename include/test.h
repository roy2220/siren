#pragma once


#include <exception>
#include <string>

#include "helper_macros.h"


#define _SIREN_TEST UNIQUE_ID(_Test)

#define SIREN_TEST(DESCRIPTION)                               \
    class _SIREN_TEST final                                   \
      : public Test                                           \
    {                                                         \
        _SIREN_TEST(const _SIREN_TEST &) = delete;            \
        void operator=(const _SIREN_TEST &) = delete;         \
                                                              \
    public:                                                   \
        explicit _SIREN_TEST() = default;                     \
                                                              \
        const char *getFileName() const noexcept override;    \
        int getLineNumber() const noexcept override;          \
        const char *getDescription() const noexcept override; \
        void run() const override;                            \
    } _SIREN_TEST;                                            \
                                                              \
                                                              \
    const char *                                              \
    _SIREN_TEST::getFileName() const noexcept                 \
    {                                                         \
        return __FILE__;                                      \
    }                                                         \
                                                              \
                                                              \
    int                                                       \
    _SIREN_TEST::getLineNumber() const noexcept               \
    {                                                         \
        return __LINE__;                                      \
    }                                                         \
                                                              \
                                                              \
    const char *                                              \
    _SIREN_TEST::getDescription() const noexcept              \
    {                                                         \
        return DESCRIPTION;                                   \
    }                                                         \
                                                              \
                                                              \
    void                                                      \
    _SIREN_TEST::run() const

#define SIREN_TEST_ASSERT(EXPRESSION)                                \
    do {                                                             \
        if (!(EXPRESSION)) {                                         \
            throw ::siren::TestAssertion(STR(EXPRESSION), __LINE__); \
        }                                                            \
    } while (false)


namespace siren {

class Test
{
    Test(const Test &) = delete;
    void operator=(const Test &) = delete;

public:
    inline explicit Test();

    virtual const char *getFileName() const noexcept = 0;
    virtual int getLineNumber() const noexcept = 0;
    virtual const char *getDescription() const noexcept = 0;
    virtual void run() const = 0;

protected:
    inline ~Test() = default;
};


class TestAssertion final
  : public std::exception
{
    TestAssertion(const TestAssertion &) = delete;
    void operator=(const TestAssertion &) = delete;

public:
    inline explicit TestAssertion(const char *, int);
    inline TestAssertion(TestAssertion &&);

    inline const char *what() const noexcept override;

private:
    std::string description_;
};


int RunTests();

void _AddTest(Test *);

}


/*
 * #include "test-inl.h"
 */


#include <utility>


namespace siren {

Test::Test()
{
    _AddTest(this);
}


TestAssertion::TestAssertion(const char *expression, int lineNumber)
{
    description_ = "SIREN_TEST_ASSERT(";
    description_ += expression;
    description_ += ") at line ";
    description_ += std::to_string(lineNumber);
}


TestAssertion::TestAssertion(TestAssertion &&other)
  : description_(std::move(other.description_))
{
}


const char *
TestAssertion::what() const noexcept
{
    return description_.data();
}

}
