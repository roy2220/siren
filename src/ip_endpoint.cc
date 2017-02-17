#include "ip_endpoint.h"

#include <cstring>

#include "async.h"
#include "scope_guard.h"


namespace siren {

sockaddr_in
IPEndpoint::ResolveName(Async *async, const char *hostName, const char *serviceName)
{
    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    addrinfo *result;
    int errorCode = async->getaddrinfo(hostName, serviceName, &hints, &result);

    if (errorCode != 0) {
        throw GAIError(errorCode);
    }

    auto scopeGuard = MakeScopeGuard([&] () -> void {
        freeaddrinfo(result);
    });

    return *reinterpret_cast<sockaddr_in *>(result->ai_addr);
}


GAIError::GAIError(int code)
{
    description_ = "getaddrinfo() failed: ";
    description_ += gai_strerror(code);
}


const char *
GAIError::what() const noexcept
{
    return description_.c_str();
}

} // namespace siren
