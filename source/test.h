#pragma once


#include <exception>
#include <iostream>
#include <string>

#include "helper_macros.h"


#define SIREN_TEST_NAME UNIQUE_NAME(Test)

#define SIREN_TEST(DESCRIPTION)                                                  \
    class SIREN_TEST_NAME final                                                  \
    {                                                                            \
        SIREN_TEST_NAME(const SIREN_TEST_NAME &) = delete;                       \
        void operator=(const SIREN_TEST_NAME &) = delete;                        \
                                                                                 \
    public:                                                                      \
        explicit SIREN_TEST_NAME();                                              \
                                                                                 \
    private:                                                                     \
        static void Do();                                                        \
    } SIREN_TEST_NAME;                                                           \
                                                                                 \
                                                                                 \
    SIREN_TEST_NAME::SIREN_TEST_NAME()                                           \
    {                                                                            \
        try {                                                                    \
            Do();                                                                \
        } catch (const std::exception &exception) {                              \
            std::cerr << __FILE__ ":" STR(__LINE__) ": " DESCRIPTION " failed: " \
            << exception.what() << std::endl;                                    \
        }                                                                        \
    }                                                                            \
                                                                                 \
                                                                                 \
    void SIREN_TEST_NAME::Do()

#define SIREN_TEST_ASSERT(EXPRESSION)                                \
    do {                                                             \
        if (!(EXPRESSION)) {                                         \
            throw ::siren::TestAssertion(STR(EXPRESSION), __LINE__); \
        }                                                            \
    } while (false)


#include <utility>


namespace siren {

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

}


/*
 * #include "test-inl.h"
 */


namespace siren {

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
