#pragma once

#include <string>

#include <ESPressio_IObserver.hpp>

namespace ESPressio::Sockets {

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
