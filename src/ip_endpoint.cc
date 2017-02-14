#include "ip_endpoint.h"

#include <cstring>

#include "async.h"
#include "scope_guard.h"


namespace siren {

sockaddr_in
IPEndpoint::ResolveName(Async *async, const char *hostName, const char *serviceName)
{
    addrinfo hints;
    std::memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    addrinfo *result;
    int errorCode = async->getaddrinfo(hostName, serviceName, &hints, &result);

    if (errorCode != 0) {
        throw GAIError(errorCode);
    }

    auto scopeGuard = MakeScopeGuard([result] () -> void {
        freeaddrinfo(result);
    });

    return *reinterpret_cast<sockaddr_in *>(result->ai_addr);
}


GAIError::GAIError(int code) noexcept
  : code_(code)
{
}


const char *
GAIError::what() const noexcept
{
    return gai_strerror(code_);
}

} // namespace siren
