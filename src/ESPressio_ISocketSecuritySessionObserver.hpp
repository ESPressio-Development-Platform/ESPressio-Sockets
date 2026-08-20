#pragma once

#include <ESPressio_IObserver.hpp>
#include <ESPressio_Security.hpp>

namespace ESPressio::Sockets {

class ISocketSecuritySessionObserver :
    public virtual Observable::IObserver {
public:
    virtual ~ISocketSecuritySessionObserver() = default;

    virtual void OnSocketSecuritySessionFaulted(
        const Security::SecurityResult&
    ) {}

    virtual void OnSocketSecuritySessionReset() {}
};

} // namespace ESPressio::Sockets
