#pragma once


#include <cstdint>
#include <exception>

#include <netinet/in.h>


namespace siren {

class Async;


class IPEndpoint final
{
public:
    std::uint32_t address;
    std::uint16_t portNumber;

    inline explicit IPEndpoint(std::uint32_t = 0, std::uint16_t = 0) noexcept;
    inline explicit IPEndpoint(const sockaddr_in &) noexcept;
    inline explicit IPEndpoint(Async *, const char *, const char *);

private:
    static sockaddr_in ResolveName(Async *, const char *, const char *);
};


class GAIError final
  : public std::exception
{
public:
    explicit GAIError(int) noexcept;

    const char *what() const noexcept override;

private:
    int code_;
};

} // namespace siren


/*
 * #include "ip_endpoint.h"
 */


#include <cassert>


namespace siren {

IPEndpoint::IPEndpoint(std::uint32_t address, std::uint16_t portNumber) noexcept
  : address(address),
    portNumber(portNumber)
{
}


IPEndpoint::IPEndpoint(const sockaddr_in &name) noexcept
  : IPEndpoint(ntohl(name.sin_addr.s_addr), ntohs(name.sin_port))
{
}


IPEndpoint::IPEndpoint(Async *async, const char *hostName, const char *serviceName)
  : IPEndpoint(ResolveName(async, hostName, serviceName))
{
    assert(async != nullptr);
}

} // namespace siren
