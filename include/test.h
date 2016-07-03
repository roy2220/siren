#pragma once


#include <exception>
#include <string>

#include "helper_macros.h"


#define SIREN_TEST_ UNIQUE_ID(Test, _)

#define SIREN_TEST(DESCRIPTION)                               \
    class SIREN_TEST_ final                                   \
      : public siren::Test_                                   \
    {                                                         \
    public:                                                   \
        explicit SIREN_TEST_();                               \
                                                              \
        const char *getFileName() const noexcept override;    \
        int getLineNumber() const noexcept override;          \
        const char *getDescription() const noexcept override; \
        void run() const override;                            \
                                                              \
    private:                                                  \
        SIREN_TEST_(const SIREN_TEST_ &) = delete;            \
        SIREN_TEST_ &operator=(const SIREN_TEST_ &) = delete; \
    } SIREN_TEST_;                                            \
                                                              \
                                                              \
    SIREN_TEST_::SIREN_TEST_()                                \
    {                                                         \
    }                                                         \
                                                              \
                                                              \
    const char *                                              \
    SIREN_TEST_::getFileName() const noexcept                 \
    {                                                         \
        return __FILE__;                                      \
    }                                                         \
                                                              \
                                                              \
    int                                                       \
    SIREN_TEST_::getLineNumber() const noexcept               \
    {                                                         \
        return __LINE__;                                      \
    }                                                         \
                                                              \
                                                              \
    const char *                                              \
    SIREN_TEST_::getDescription() const noexcept              \
    {                                                         \
        return DESCRIPTION;                                   \
    }                                                         \
                                                              \
                                                              \
    void                                                      \
    SIREN_TEST_::run() const

#define SIREN_TEST_ASSERT(EXPRESSION)                                 \
    do {                                                              \
        if (!(EXPRESSION)) {                                          \
            throw ::siren::TestAssertion_(STR(EXPRESSION), __LINE__); \
        }                                                             \
    } while (false)


namespace siren {

class Test_
{
public:
    virtual const char *getFileName() const noexcept = 0;
    virtual int getLineNumber() const noexcept = 0;
    virtual const char *getDescription() const noexcept = 0;
    virtual void run() const = 0;

protected:
    inline explicit Test_();

    ~Test_() = default;

private:
    Test_(const Test_ &) = delete;
    Test_ &operator=(const Test_ &) = delete;
};


class TestAssertion_ final
  : public std::exception
{
public:
    inline explicit TestAssertion_(const char *, int);

    inline const char *what() const noexcept override;

    TestAssertion_(TestAssertion_ &&) = default;
    TestAssertion_ &operator=(TestAssertion_ &&) = default;

private:
    std::string description_;

    TestAssertion_(const TestAssertion_ &) = delete;
    TestAssertion_ &operator=(const TestAssertion_ &) = delete;
};


int RunTests();

void AddTest_(Test_ *);

}


/*
 * #include "test-inl.h"
 */


#include <utility>


namespace siren {

Test_::Test_()
{
    AddTest_(this);
}


TestAssertion_::TestAssertion_(const char *expression, int lineNumber)
{
    description_ = "SIREN_TEST_ASSERT(";
    description_ += expression;
    description_ += ") at line ";
    description_ += std::to_string(lineNumber);
}


const char *
TestAssertion_::what() const noexcept
{
    return description_.data();
}

}
