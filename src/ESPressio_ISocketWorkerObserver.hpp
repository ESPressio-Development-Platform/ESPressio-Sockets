#pragma once

#include <string>

#include <ESPressio_IObserver.hpp>

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members: none; polymorphic interface/object includes vptr storage where not supplied by a base.
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
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
