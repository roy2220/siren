#pragma once


#include <cstddef>
#include <exception>
#include <string>

#include "utility.h"


#define SIREN__TEST_IMPL SIREN_CONCAT(SirenTest, __LINE__)

#define SIREN_TEST(DESCRIPTION)                                         \
    class SIREN__TEST_IMPL final                                        \
      : public ::siren::detail::Test                                    \
    {                                                                   \
    public:                                                             \
        explicit SIREN__TEST_IMPL() {}                                  \
                                                                        \
        const char *getFileName() const noexcept override {             \
            return __FILE__;                                            \
        }                                                               \
                                                                        \
        unsigned int getLineNumber() const noexcept override {          \
            return __LINE__;                                            \
        }                                                               \
                                                                        \
        const char *getDescription() const noexcept override {          \
            return (DESCRIPTION);                                       \
        }                                                               \
                                                                        \
        void run() override {                                           \
            Run();                                                      \
        }                                                               \
                                                                        \
    private:                                                            \
        static void Run();                                              \
                                                                        \
        SIREN__TEST_IMPL(const SIREN__TEST_IMPL &) = delete;            \
        SIREN__TEST_IMPL &operator=(const SIREN__TEST_IMPL &) = delete; \
    } SIREN__TEST_IMPL;                                                 \
                                                                        \
                                                                        \
    void                                                                \
    SIREN__TEST_IMPL::Run()

#define SIREN_TEST_ASSERT(EXPRESSION)                                                     \
    do {                                                                                  \
        if (!(EXPRESSION)) {                                                              \
            throw ::siren::detail::TestAssertionFailure(SIREN_STR(EXPRESSION), __LINE__); \
        }                                                                                 \
    } while (false)


namespace siren {

std::size_t GetNumberOfTests() noexcept;
std::size_t RunTests() noexcept;


namespace detail {

class Test
{
public:
    virtual const char *getFileName() const noexcept = 0;
    virtual unsigned int getLineNumber() const noexcept = 0;
    virtual const char *getDescription() const noexcept = 0;
    virtual void run() = 0;

protected:
    explicit Test();

    ~Test() = default;

private:
    Test(const Test &) = delete;
    Test &operator=(const Test &) = delete;
};


class TestAssertionFailure final
  : public std::exception
{
public:
    explicit TestAssertionFailure(const char *, unsigned int);

    const char *what() const noexcept override;

private:
    std::string description_;
};

} // namespace detail

} // namespace siren
