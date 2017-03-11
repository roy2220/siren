#include "tcp_socket.h"

#include <cerrno>
#include <algorithm>
#include <system_error>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "assert.h"
#include "loop.h"
#include "stream.h"


namespace siren {

TCPSocket::TCPSocket(Loop *loop)
  : loop_(loop)
{
    SIREN_ASSERT(loop != nullptr);
    initialize();
}


TCPSocket::TCPSocket(Loop *loop, int fd) noexcept
  : loop_(loop),
    fd_(fd)
{
}


TCPSocket::TCPSocket(TCPSocket &&other) noexcept
  : loop_(other.loop_)
{
    other.move(this);
}


TCPSocket::~TCPSocket()
{
    if (isValid()) {
        finalize();
    }
}


TCPSocket &
TCPSocket::operator=(TCPSocket &&other) noexcept
{
    if (&other != this) {
        finalize();
        loop_ = other.loop_;
        other.move(this);
    }

    return *this;
}


void
TCPSocket::initialize()
{
    fd_ = loop_->socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (fd_ < 0) {
        throw std::system_error(errno, std::system_category(), "socket() failed");
    }
}


void
TCPSocket::finalize() noexcept
{
    if (loop_->close(fd_) < 0 && errno != EINTR) {
        std::perror("close() failed");
        std::terminate();
    }
}


void
TCPSocket::move(TCPSocket *other) noexcept
{
    other->fd_ = fd_;
    fd_ = -1;
}


void
TCPSocket::setReuseAddress(bool reuseAddress)
{
    int onOff = reuseAddress;

    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &onOff, sizeof(onOff)) < 0) {
        throw std::system_error(errno, std::system_category(), "setsockopt(SO_REUSEADDR) failed");
    }
}


void
TCPSocket::setNoDelay(bool noDelay)
{
    int onOff = noDelay;

    if (setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &onOff, sizeof(onOff)) < 0) {
        throw std::system_error(errno, std::system_category(), "setsockopt(TCP_NODELAY) failed");
    }
}


void
TCPSocket::setLinger(bool linger1, int interval)
{
    linger value;
    value.l_onoff = linger1;
    value.l_linger = interval;

    if (setsockopt(fd_, SOL_SOCKET, SO_LINGER, &value, sizeof(value)) < 0) {
        throw std::system_error(errno, std::system_category(), "setsockopt(SO_LINGER) failed");
    }
}


void
TCPSocket::setKeepAlive(bool keepAlive, int interval)
{
    int onOff = keepAlive;

    if (setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &onOff, sizeof(onOff)) < 0) {
        throw std::system_error(errno, std::system_category(), "setsockopt(SO_KEEPALIVE) failed");
    }

    if (keepAlive) {
        if (setsockopt(fd_, IPPROTO_TCP, TCP_KEEPIDLE, &interval, sizeof(interval)) < 0) {
            throw std::system_error(errno, std::system_category()
                                    , "setsockopt(TCP_KEEPIDLE) failed");
        }

        int count = 3;

        if (setsockopt(fd_, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count)) < 0) {
            throw std::system_error(errno, std::system_category()
                                    , "setsockopt(TCP_KEEPCNT) failed");
        }

        interval = std::max(interval / count, 1);

        if (setsockopt(fd_, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval)) < 0) {
            throw std::system_error(errno, std::system_category()
                                    , "setsockopt(TCP_KEEPINTVL) failed");
        }
    }
}


void
TCPSocket::setReceiveTimeout(long receiveTimeout)
{
    SIREN_ASSERT(receiveTimeout >= 0);
    timeval time;
    time.tv_sec = receiveTimeout / 1000;
    time.tv_usec = (receiveTimeout % 1000) * 1000;

    if (loop_->setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time)) < 0) {
        throw std::system_error(errno, std::system_category(), "setsockopt(SO_RCVTIMEO) failed");
    }
}


void
TCPSocket::setSendTimeout(long sendTimeout)
{
    SIREN_ASSERT(sendTimeout >= 0);
    timeval time;
    time.tv_sec = sendTimeout / 1000;
    time.tv_usec = (sendTimeout % 1000) * 1000;

    if (loop_->setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &time, sizeof(time)) < 0) {
        throw std::system_error(errno, std::system_category(), "setsockopt(SO_SNDTIMEO) failed");
    }
}


void
TCPSocket::setReceiveBufferSize(int receiveBufferSize)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &receiveBufferSize, sizeof(receiveBufferSize)) < 0) {
        throw std::system_error(errno, std::system_category(), "setsockopt(SO_RCVBUF) failed");
    }
}


void
TCPSocket::setSendBufferSize(int sendBufferSize)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &sendBufferSize, sizeof(sendBufferSize)) < 0) {
        throw std::system_error(errno, std::system_category(), "setsockopt(SO_SNDBUF) failed");
    }
}


void
TCPSocket::listen(const IPEndpoint &ipEndpoint, int backlog)
{
    SIREN_ASSERT(isValid());
    sockaddr_in name;
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(ipEndpoint.address);
    name.sin_port = htons(ipEndpoint.portNumber);

    if (bind(fd_, reinterpret_cast<sockaddr *>(&name), sizeof(name)) < 0) {
        throw std::system_error(errno, std::system_category(), "bind() failed");
    }

    if (::listen(fd_, backlog) < 0) {
        throw std::system_error(errno, std::system_category(), "listen() failed");
    }
}


TCPSocket
TCPSocket::accept(IPEndpoint *ipEndpoint)
{
    SIREN_ASSERT(isValid());
    sockaddr_in name;
    socklen_t nameSize = sizeof(name);
    int subFD = loop_->accept(fd_, reinterpret_cast<sockaddr *>(&name), &nameSize);

    if (subFD < 0) {
        throw std::system_error(errno, std::system_category(), "accept() failed");
    }

    if (ipEndpoint != nullptr) {
        *ipEndpoint = IPEndpoint(name);
    }

    return TCPSocket(loop_, subFD);
}


void
TCPSocket::connect(const IPEndpoint &ipEndpoint)
{
    SIREN_ASSERT(isValid());
    sockaddr_in name;
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(ipEndpoint.address);
    name.sin_port = htons(ipEndpoint.portNumber);

    if (loop_->connect(fd_, reinterpret_cast<sockaddr *>(&name), sizeof(name)) < 0) {
        throw std::system_error(errno, std::system_category(), "connect() failed");
    }
}


IPEndpoint
TCPSocket::getLocalEndpoint() const
{
    SIREN_ASSERT(isValid());
    sockaddr_in name;
    socklen_t nameSize = sizeof(name);

    if (getsockname(fd_, reinterpret_cast<sockaddr *>(&name), &nameSize) < 0) {
        throw std::system_error(errno, std::system_category(), "getsockname() failed");
    }

    return IPEndpoint(name);
}


IPEndpoint
TCPSocket::getRemoteEndpoint() const
{
    SIREN_ASSERT(isValid());
    sockaddr_in name;
    socklen_t nameSize = sizeof(name);

    if (getpeername(fd_, reinterpret_cast<sockaddr *>(&name), &nameSize) < 0) {
        throw std::system_error(errno, std::system_category(), "getpeername() failed");
    }

    return IPEndpoint(name);
}


std::size_t
TCPSocket::read(void *buffer, std::size_t bufferSize)
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(buffer != nullptr || bufferSize == 0);
    ssize_t numberOfBytes = loop_->recv(fd_, buffer, bufferSize, 0);

    if (numberOfBytes < 0) {
        throw std::system_error(errno, std::system_category(), "recv() failed");
    }

    return numberOfBytes;
}


std::size_t
TCPSocket::write(const void *data, std::size_t dataSize)
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(data != nullptr || dataSize == 0);
    ssize_t numberOfBytes = loop_->send(fd_, data, dataSize, MSG_NOSIGNAL);

    if (numberOfBytes < 0) {
        throw std::system_error(errno, std::system_category(), "send() failed");
    }

    return numberOfBytes;
}


std::size_t
TCPSocket::read(Stream *stream)
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(stream != nullptr);
    std::size_t numberOfBytes = read(stream->getBuffer(), stream->getBufferSize());
    stream->commitBuffer(numberOfBytes);
    return numberOfBytes;
}


std::size_t
TCPSocket::write(Stream *stream)
{
    SIREN_ASSERT(isValid());
    SIREN_ASSERT(stream != nullptr);
    std::size_t numberOfBytes = write(stream->getData(), stream->getDataSize());
    stream->discardData(numberOfBytes);
    return numberOfBytes;
}


void
TCPSocket::closeRead()
{
    SIREN_ASSERT(isValid());

    if (shutdown(fd_, SHUT_RD) < 0) {
        throw std::system_error(errno, std::system_category(), "shutdown(SHUT_RD) failed");
    }
}


void
TCPSocket::closeWrite()
{
    SIREN_ASSERT(isValid());

    if (shutdown(fd_, SHUT_WR) < 0) {
        throw std::system_error(errno, std::system_category(), "shutdown(SHUT_WR) failed");
    }
}

} // namespace siren
