#pragma once

#include <ESPressio_ISocketSecuritySessionObserver.hpp>
#include <ESPressio_SocketSecuritySession.hpp>

#include "ESPressio_SocketEvents.hpp"

namespace ESPressio::Event {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes [0 bytes dynamic allocation]
 * Members:
 * - _observerHandle (Observable::ObserverHandlePtr): sizeof(Observable::ObserverHandlePtr) [0 bytes dynamic allocation]
 * - _initialized (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 4 bytes known bases + 1 bytes known members + sizeof(Observable::ObserverHandlePtr) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SocketSecuritySessionEventBridge final :
    public Sockets::ISocketSecuritySessionObserver {
private:
    Observable::ObserverHandlePtr _observerHandle;
    bool _initialized = false;

public:
    SocketSecuritySessionEventBridge() = default;
    SocketSecuritySessionEventBridge(const SocketSecuritySessionEventBridge&) = delete;
    SocketSecuritySessionEventBridge& operator=(const SocketSecuritySessionEventBridge&) = delete;

    bool Initialize(Sockets::SocketSecuritySession& session) {
        if (_initialized) return true;
        _observerHandle = session.RegisterObserver(this);
        _initialized = static_cast<bool>(_observerHandle);
        return _initialized;
    }

    void Shutdown() {
        _observerHandle.reset();
        _initialized = false;
    }

    bool IsInitialized() const { return _initialized; }

    void OnSocketSecuritySessionFaulted(const Security::SecurityResult& result) override {
        (new SocketSecuritySessionFaultedEvent(result))->Queue();
    }

    void OnSocketSecuritySessionReset() override {
        (new SocketSecuritySessionResetEvent())->Queue();
    }
};

} // namespace ESPressio::Event
