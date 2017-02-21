#pragma once


#include <cstddef>
#include <memory>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "thread_pool.h"


namespace siren {

class Event;
class Loop;
namespace detail { struct AsyncTask; }


class Async final
{
public:
    inline bool isValid() const noexcept;

    explicit Async(Loop *, std::size_t = 0);
    Async(Async &&) noexcept;
    ~Async();
    Async &operator=(Async &&) noexcept;

    int getaddrinfo(const char *, const char *, const addrinfo *, addrinfo **);
    int getnameinfo(const sockaddr *, socklen_t, char *, socklen_t, char *, socklen_t, int);
    int open(const char *, int, mode_t = 0);
    ssize_t read(int, void *, size_t);
    ssize_t write(int, const void *, size_t);
    ssize_t readv(int, const iovec *, int);
    ssize_t writev(int, const iovec *, int);
    int close(int);
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


namespace siren {

bool
Async::isValid() const noexcept
{
    return threadPool_ != nullptr && fiberHandle_ != nullptr;
}

} // namespace siren
