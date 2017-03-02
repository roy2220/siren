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
    inline int getFD() const noexcept;

    explicit TCPSocket(Loop *);
    TCPSocket(TCPSocket &&) noexcept;
    ~TCPSocket();
    TCPSocket &operator=(TCPSocket &&) noexcept;

    void setReuseAddress(bool);
    void setNoDelay(bool);
    void setLinger(bool, int);
    void setKeepAlive(bool, int);
    void setReceiveTimeout(long);
    void setSendTimeout(long);
    void setReceiveBufferSize(int);
    void setSendBufferSize(int);
    void listen(const IPEndpoint &, int = 511);
    TCPSocket accept(IPEndpoint * = nullptr);
    void connect(const IPEndpoint &);
    IPEndpoint getLocalEndpoint() const;
    IPEndpoint getRemoteEndpoint() const;
    std::size_t read(Stream *);
    std::size_t write(Stream *);
    void closeRead();
    void closeWrite();

private:
    Loop *loop_;
    int fd_;

    explicit TCPSocket(Loop *, int) noexcept;

    void initialize();
    void finalize() noexcept;
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


int
TCPSocket::getFD() const noexcept
{
    return fd_;
}

} // namespace siren
