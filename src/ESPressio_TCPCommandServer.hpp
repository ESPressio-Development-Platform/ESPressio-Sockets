#pragma once

#if !__has_include(<ESPressio_Command.hpp>)
#error "TCPCommandServer requires ESPressio Command >= 0.2.0 < 1.0.0."
#endif

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include <WiFiClient.h>
#include <WiFiServer.h>

#include "ESPressio_SocketCommandSession.hpp"
#include "ESPressio_SocketStreamHelpers.hpp"
#include "ESPressio_SocketTypes.hpp"
#include "ESPressio_SocketWorker.hpp"

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Members:
 * - Port (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * - MaximumClients (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * - Session (SocketCommandSessionConfig): 12 bytes [0 bytes dynamic allocation]
 * - Worker (SocketWorkerConfig): 16 bytes [0 bytes dynamic allocation]
 * Total Memory: 36 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
struct TCPCommandServerConfig {
    uint16_t Port = 0;
    std::size_t MaximumClients = ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS;
    SocketCommandSessionConfig Session;
    SocketWorkerConfig Worker;
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 36 bytes [SocketWorker: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Members:
 * - _server (std::unique_ptr<WiFiServer>): 4 bytes [owned object: sizeof(WiFiServer) (target/toolchain dependent)]
 * - _clients (std::array<ClientState, ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS>): ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS * (153 bytes known/aligned storage + sizeof(WiFiClient) (target/toolchain dependent)) [elements: Session: _metadata: Transport: Capacity + 1 bytes when capacity exceeds 15-byte SSO; elements: Session: _metadata: RemoteAddress: Capacity + 1 bytes when capacity exceeds 15-byte SSO; elements: Session: _line: Capacity + 1 bytes when capacity exceeds 15-byte SSO; elements: Session: _structured: Capacity * (1 bytes) element storage]
 * - _config (TCPCommandServerConfig): 36 bytes [0 bytes dynamic allocation]
 * - _registry (Command::CommandRegistry*): 4 bytes [0 bytes dynamic allocation]
 * - _policy (SocketCommandPolicyHandler): 4 bytes [0 bytes dynamic allocation]
 * - _observer (SocketCommandResultObserver): 4 bytes [0 bytes dynamic allocation]
 * - _clientsMutex (std::mutex): 4 bytes [native synchronization state may allocate platform resources lazily]
 * - _nextSessionID (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * - _initialized (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 101 bytes known/aligned storage + ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS * (153 bytes known/aligned storage + sizeof(WiFiClient) (target/toolchain dependent)) [SocketWorker: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _server: owned object: sizeof(WiFiServer) (target/toolchain dependent); _clients: elements: Session: _metadata: Transport: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _clients: elements: Session: _metadata: RemoteAddress: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _clients: elements: Session: _line: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _clients: elements: Session: _structured: Capacity * (1 bytes) element storage; _clientsMutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class TCPCommandServer final : private SocketWorker {
public:
    TCPCommandServer() = default;
    TCPCommandServer(const TCPCommandServer&) = delete;
    TCPCommandServer& operator=(const TCPCommandServer&) = delete;
    ~TCPCommandServer() override { Shutdown(); }

    bool Initialize(
        const TCPCommandServerConfig& config,
        Command::CommandRegistry& registry = Command::CommandRegistry::GetInstance()
    ) {
        if (_initialized) return true;
        if (config.Port == 0 || config.MaximumClients == 0 ||
            config.MaximumClients > _clients.size() ||
            config.Session.MaximumRequestBytes == 0) return false;

        _config = config;
        _registry = &registry;
        _server = std::make_unique<WiFiServer>(config.Port);
        _server->begin();
        _server->setNoDelay(true);

        if (!StartWorker("ESPressioCmdTCP", config.Worker)) {
            _server->end();
            _server.reset();
            _registry = nullptr;
            return false;
        }
        _initialized = true;
        return true;
    }

    void Shutdown() {
        if (!_initialized && _server == nullptr) return;
        StopWorker();
        std::lock_guard<std::mutex> lock(_clientsMutex);
        for (auto& state : _clients) ResetClient(state);
        if (_server) {
            _server->end();
            _server.reset();
        }
        _registry = nullptr;
        _initialized = false;
    }

    bool GetIsInitialized() const noexcept { return _initialized; }

    std::size_t GetConnectedClientCount() const {
        std::lock_guard<std::mutex> lock(_clientsMutex);
        std::size_t count = 0;
        for (const auto& state : _clients) {
            if (state.Active && state.Client.connected()) ++count;
        }
        return count;
    }

    void SetPolicy(SocketCommandPolicyHandler policy) {
        std::lock_guard<std::mutex> lock(_clientsMutex);
        _policy = std::move(policy);
        for (auto& state : _clients) if (state.Active) state.Session.SetPolicy(_policy);
    }

    void SetResultObserver(SocketCommandResultObserver observer) {
        std::lock_guard<std::mutex> lock(_clientsMutex);
        _observer = std::move(observer);
        for (auto& state : _clients) if (state.Active) state.Session.SetResultObserver(_observer);
    }

protected:
    void OnWorkerIteration() override {
        std::lock_guard<std::mutex> lock(_clientsMutex);
        AcceptClientLocked();
        std::array<uint8_t, 512> buffer{};

        for (std::size_t i = 0; i < _config.MaximumClients; ++i) {
            auto& state = _clients[i];
            if (!state.Active) continue;
            if (!state.Client.connected()) {
                ResetClient(state);
                continue;
            }

            while (state.Client.available() > 0) {
                const int count = state.Client.read(buffer.data(), buffer.size());
                if (count <= 0) break;
                if (!state.Session.Feed(buffer.data(), static_cast<std::size_t>(count))) {
                    if (_config.Session.DisconnectOnProtocolError) {
                        ResetClient(state);
                        break;
                    }
                }
            }
        }
    }

private:
/**
 * ESPressio Memory Audit
 * Members:
 * - Client (WiFiClient): sizeof(WiFiClient) (target/toolchain dependent) [0 bytes dynamic allocation]
 * - Session (SocketCommandSession): 144 bytes [_metadata: Transport: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _metadata: RemoteAddress: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _line: Capacity + 1 bytes when capacity exceeds 15-byte SSO; _structured: Capacity * (1 bytes) element storage]
 * - ID (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * - Active (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 153 bytes known/aligned storage + sizeof(WiFiClient) (target/toolchain dependent) [Session: _metadata: Transport: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Session: _metadata: RemoteAddress: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Session: _line: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Session: _structured: Capacity * (1 bytes) element storage]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct ClientState {
        WiFiClient Client;
        SocketCommandSession Session;
        uint64_t ID = 0;
        bool Active = false;
    };

    std::unique_ptr<WiFiServer> _server;
    std::array<ClientState, ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS> _clients;
    TCPCommandServerConfig _config;
    Command::CommandRegistry* _registry = nullptr;
    SocketCommandPolicyHandler _policy;
    SocketCommandResultObserver _observer;
    mutable std::mutex _clientsMutex;
    uint64_t _nextSessionID = 1;
    bool _initialized = false;

    static void ResetClient(ClientState& state) {
        state.Session.Shutdown();
        state.Client.stop();
        state.ID = 0;
        state.Active = false;
    }

    void AcceptClientLocked() {
        if (!_server || _registry == nullptr) return;
        WiFiClient incoming = _server->available();
        if (!incoming) return;

        for (std::size_t i = 0; i < _config.MaximumClients; ++i) {
            auto& state = _clients[i];
            if (state.Active && state.Client.connected()) continue;
            ResetClient(state);
            state.Client = incoming;
            state.ID = _nextSessionID++;
            if (_nextSessionID == 0) _nextSessionID = 1;

            SocketCommandMetadata metadata;
            metadata.Transport = "tcp";
            metadata.RemoteAddress = std::string(state.Client.remoteIP().toString().c_str());
            metadata.RemotePort = state.Client.remotePort();
            metadata.SessionID = state.ID;

            ClientState* clientState = &state;
            const bool initialized = state.Session.Initialize(
                *_registry,
                _config.Session,
                std::move(metadata),
                [clientState](const uint8_t* data, std::size_t size) {
                    if (!clientState->Active || !clientState->Client.connected()) return false;
                    return WriteAll(clientState->Client, data, size);
                }
            );

            if (!initialized) {
                ResetClient(state);
                return;
            }
            state.Session.SetPolicy(_policy);
            state.Session.SetResultObserver(_observer);
            state.Active = true;
            return;
        }

        incoming.stop();
    }
};

}
