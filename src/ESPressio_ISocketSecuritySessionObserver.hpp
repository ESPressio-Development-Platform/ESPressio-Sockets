#pragma once

#include <ESPressio_IObserver.hpp>
#include <ESPressio_Security.hpp>

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members: none; polymorphic interface/object includes vptr storage where not supplied by a base.
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
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
