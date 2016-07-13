#pragma once


#include <exception>
#include <string>

#include "helper_macros.h"


#define SIREN_TEST_ UNIQUE_ID(Test_)

#define SIREN_TEST(DESCRIPTION)                               \
    class SIREN_TEST_ final                                   \
      : public siren::Test_                                   \
    {                                                         \
        SIREN_TEST_(const SIREN_TEST_ &) = delete;            \
        void operator=(const SIREN_TEST_ &) = delete;         \
                                                              \
    public:                                                   \
        explicit SIREN_TEST_();                               \
                                                              \
        const char *getFileName() const noexcept override;    \
        int getLineNumber() const noexcept override;          \
        const char *getDescription() const noexcept override; \
        void run() const override;                            \
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
    Test_(const Test_ &) = delete;
    void operator=(const Test_ &) = delete;

public:
    virtual const char *getFileName() const noexcept = 0;
    virtual int getLineNumber() const noexcept = 0;
    virtual const char *getDescription() const noexcept = 0;
    virtual void run() const = 0;

protected:
    inline explicit Test_();
    inline ~Test_();
};


class TestAssertion_ final
  : public std::exception
{
    TestAssertion_(const TestAssertion_ &) = delete;
    void operator=(const TestAssertion_ &) = delete;

public:
    inline explicit TestAssertion_(const char *, int);
    inline TestAssertion_(TestAssertion_ &&);

    inline const char *what() const noexcept override;

private:
    std::string description_;
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


Test_::~Test_()
{
}


TestAssertion_::TestAssertion_(const char *expression, int lineNumber)
{
    description_ = "SIREN_TEST_ASSERT(";
    description_ += expression;
    description_ += ") at line ";
    description_ += std::to_string(lineNumber);
}


TestAssertion_::TestAssertion_(TestAssertion_ &&other)
  : description_(std::move(other.description_))
{
}


const char *
TestAssertion_::what() const noexcept
{
    return description_.data();
}

}
