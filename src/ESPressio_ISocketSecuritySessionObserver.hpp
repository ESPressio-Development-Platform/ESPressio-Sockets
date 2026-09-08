#pragma once

#include <ESPressio_IObserver.hpp>
#include <ESPressio_Security.hpp>

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members: none; polymorphic/virtual-base object metadata is included in the total.
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
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
