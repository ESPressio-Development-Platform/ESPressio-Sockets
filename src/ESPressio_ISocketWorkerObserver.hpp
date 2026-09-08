#pragma once

#include <string>

#include <ESPressio_IObserver.hpp>

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members: none; polymorphic/virtual-base object metadata is included in the total.
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class ISocketWorkerObserver :
    public virtual Observable::IObserver {
public:
    virtual ~ISocketWorkerObserver() = default;

    virtual void OnSocketWorkerStarted(
        const char*
    ) {}

    virtual void OnSocketWorkerStartFailed(
        const char*
    ) {}

    virtual void OnSocketWorkerStopped() {}
};

} // namespace ESPressio::Sockets
