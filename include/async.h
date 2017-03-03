#pragma once


#include <cstddef>
#include <memory>
#include <type_traits>

#include "thread_pool.h"


namespace siren {

class Event;
class Loop;
namespace detail { struct AsyncTask; }


class Async final
{
public:
    inline bool isValid() const noexcept;

    template <class T, class ...U>
    std::enable_if_t<std::is_void<std::result_of_t<T(U ...)>>::value, void> callFunction(T &&, U ...);

    template <class T, class ...U>
    std::enable_if_t<!std::is_void<std::result_of_t<T(U ...)>>::value
                     && !std::is_reference<std::result_of_t<T(U ...)>>::value
                     , std::result_of_t<T(U ...)>> callFunction(T &&, U ...);

    explicit Async(Loop *, std::size_t = 0);
    Async(Async &&) noexcept;
    ~Async();
    Async &operator=(Async &&) noexcept;

    void executeTask(const std::function<void ()> &);
    void executeTask(std::function<void ()> &&);

private:
    typedef detail::AsyncTask Task;

    std::unique_ptr<ThreadPool> threadPool_;
    Loop *loop_;
    void *fiberHandle_;
    std::size_t taskCount_;

    static void EventTrigger(ThreadPool *, Loop *) noexcept;

    void initialize();
    void finalize() noexcept;
    void move(Async *) noexcept;
    void waitForTask(Task *);
};

} // namespace siren


/*
 * #include "async-inl.h"
 */


#include <cerrno>
#include <new>
#include <tuple>
#include <utility>

#include "utility.h"


namespace siren {

bool
Async::isValid() const noexcept
{
    return threadPool_ != nullptr && fiberHandle_ != nullptr;
}


template <class T, class ...U>
std::enable_if_t<std::is_void<std::result_of_t<T(U ...)>>::value, void>
Async::callFunction(T &&procedure, U ...argument)
{
    typedef std::tuple<U ...> V;

    struct {
        T &&procedure;
        V arguments;
        int errorNumber;
    } context = {
        std::forward<T>(procedure),
        {argument...},
        0,
    };

    executeTask([&context] () -> void {
        struct ErrorNumberCapturer {
            int *errorNumber;
            ~ErrorNumberCapturer() { *errorNumber = errno; }
        } errorNumberCapturer = {&context.errorNumber};

        ApplyFunction(std::forward<T>(context.procedure), context.arguments);
    });

    errno = context.errorNumber;
}


template <class T, class ...U>
std::enable_if_t<!std::is_void<std::result_of_t<T(U ...)>>::value
                 && !std::is_reference<std::result_of_t<T(U ...)>>::value
                 , std::result_of_t<T(U ...)>>
Async::callFunction(T &&function, U ...argument)
{
    typedef std::tuple<U ...> V;
    typedef std::result_of_t<T(U ...)> W;

    struct {
        T &&function;
        V arguments;
        alignas(alignof(W)) char result[sizeof(W)];
        int errorNumber;
    } context = {
        std::forward<T>(function),
        {argument...},
        {},
        0,
    };

    executeTask([&context] () -> void {
        struct ErrorNumberCapturer {
            int *errorNumber;
            ~ErrorNumberCapturer() { *errorNumber = errno; }
        } errorNumberCapturer = {&context.errorNumber};

        new (context.result) W(ApplyFunction(std::forward<T>(context.function)
                                             , context.arguments));
    });

    errno = context.errorNumber;
    return *reinterpret_cast<W *>(context.result);
}

} // namespace siren
