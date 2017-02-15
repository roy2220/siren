#pragma once


#include <cstddef>

#include "ip_endpoint.h"


namespace siren {

class Loop;
class Stream;


class TCPSocket final
{
public:
    inline bool isValid() const noexcept;

    explicit TCPSocket(Loop *);
    TCPSocket(TCPSocket &&) noexcept;
    ~TCPSocket();
    TCPSocket &operator=(TCPSocket &&) noexcept;

    void setReuseAddress(bool);
    void setNoDelay(bool);
    void setLinger(bool, int);
    void setKeepAlive(bool, int);
    void listen(const IPEndpoint &, int = 511);
    TCPSocket accept(IPEndpoint * = nullptr, int = -1);
    void connect(const IPEndpoint &, int = -1);
    IPEndpoint getLocalEndpoint() const;
    IPEndpoint getRemoteEndpoint() const;
    std::size_t read(Stream *, int = -1);
    std::size_t write(Stream *, int = -1);
    void closeRead();
    void closeWrite();

private:
    Loop *loop_;
    int fd_;

    explicit TCPSocket(Loop *, int) noexcept;

    void initialize();
    void finalize();
    void move(TCPSocket *) noexcept;
};

} // namespace siren


/*
 * #include "tcp_socket-inl.h"
 */


namespace siren {

bool
TCPSocket::isValid() const noexcept
{
    return fd_ >= 0;
}

} // namespace siren
