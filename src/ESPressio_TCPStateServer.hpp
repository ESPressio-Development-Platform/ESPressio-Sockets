#pragma once

#if !__has_include(<ESPressio_State.hpp>)
#error "TCPStateServer requires ESPressio State. Add the active ESPressio-State dependency when using this optional integration."
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include <WiFiClient.h>
#include <WiFiServer.h>

#include "ESPressio_SocketStateSession.hpp"
#include "ESPressio_SocketStreamHelpers.hpp"
#include "ESPressio_SocketTypes.hpp"
#include "ESPressio_SocketWorker.hpp"

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Members:
 * - Port (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * - MaximumClients (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * - Session (SocketStateSessionConfig): 24 bytes [0 bytes dynamic allocation]
 * - Worker (SocketWorkerConfig): 16 bytes [0 bytes dynamic allocation]
 * Total Memory: 48 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
template<
    typename TContract,
    std::size_t TMaximumRemoteDevices,
    std::size_t TSubscriptionCapacity
>
struct TCPStateServerConfig final {
    uint16_t Port = 0;
    std::size_t MaximumClients = ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS;
    SocketStateSessionConfig Session;
    SocketWorkerConfig Worker;
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 36 bytes [SocketWorker: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Members:
 * - _server (std::unique_ptr<WiFiServer>): 4 bytes [owned object: sizeof(WiFiServer) (target/toolchain dependent)]
 * - _clients (std::array<ClientState, ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS>): ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS * (9 bytes known/aligned storage + sizeof(WiFiClient) (target/toolchain dependent) + 145 bytes known/aligned storage + sizeof(State::StatePublishedObserverPack<SocketStateSession<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>, TContract>) (target/toolchain dependent)) [elements: StateSession: _subscribers: _subscribers: Capacity * (17 bytes known/aligned storage + TContract::StateCount * (1 bytes)) element storage; elements: StateSession: _subscribers: _mutex: native synchronization state may allocate platform resources lazily; elements: StateSession: _subscribers: _observerMutex: native synchronization state may allocate platform resources lazily; elements: StateSession: _subscribers: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; elements: StateSession: _decoder: _buffer: Capacity * (1 bytes) element storage; elements: StateSession: _publisherHandle: owned object: 4 bytes; elements: StateSession: _subscriptionHandle: owned object: 4 bytes; elements: StateSession: _mutex: native synchronization state may allocate platform resources lazily]
 * - _config (Config): 1 bytes [0 bytes dynamic allocation]
 * - _publisher (Publisher*): 4 bytes [0 bytes dynamic allocation]
 * - _remote (RemoteManager*): 4 bytes [0 bytes dynamic allocation]
 * - _subscriptions (Subscriptions*): 4 bytes [0 bytes dynamic allocation]
 * - _clientsMutex (std::mutex): 4 bytes [native synchronization state may allocate platform resources lazily]
 * - _nextSessionID (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * - _initialized (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 69 bytes known/aligned storage + ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS * (9 bytes known/aligned storage + sizeof(WiFiClient) (target/toolchain dependent) + 145 bytes known/aligned storage + sizeof(State::StatePublishedObserverPack<SocketStateSession<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>, TContract>) (target/toolchain dependent)) [SocketWorker: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _server: owned object: sizeof(WiFiServer) (target/toolchain dependent); _clients: elements: StateSession: _subscribers: _subscribers: Capacity * (17 bytes known/aligned storage + TContract::StateCount * (1 bytes)) element storage; _clients: elements: StateSession: _subscribers: _mutex: native synchronization state may allocate platform resources lazily; _clients: elements: StateSession: _subscribers: _observerMutex: native synchronization state may allocate platform resources lazily; _clients: elements: StateSession: _subscribers: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; _clients: elements: StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _clients: elements: StateSession: _decoder: _buffer: Capacity * (1 bytes) element storage; _clients: elements: StateSession: _publisherHandle: owned object: 4 bytes; _clients: elements: StateSession: _subscriptionHandle: owned object: 4 bytes; _clients: elements: StateSession: _mutex: native synchronization state may allocate platform resources lazily; _clientsMutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
template<
    typename TContract,
    std::size_t TMaximumRemoteDevices,
    std::size_t TSubscriptionCapacity
>
class TCPStateServer final : private SocketWorker {
public:
    using Publisher = State::StatePublisher<TContract>;
    using RemoteManager = State::RemoteStateManager<TContract, TMaximumRemoteDevices>;
    using Subscriptions = State::StateSubscriptionRegistry<TSubscriptionCapacity>;
    using Session = SocketStateSession<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>;
    using Config = TCPStateServerConfig<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>;

    TCPStateServer() = default;
    TCPStateServer(const TCPStateServer&) = delete;
    TCPStateServer& operator=(const TCPStateServer&) = delete;
    ~TCPStateServer() override { Shutdown(); }

    bool Initialize(
        const Config& config,
        Publisher& publisher,
        RemoteManager& remote,
        Subscriptions& subscriptions
    ) {
        if (_initialized) return true;
        if (config.Port == 0 || config.MaximumClients == 0 ||
            config.MaximumClients > _clients.size() ||
            config.Session.MaximumProtocolMessageBytes == 0) {
            return false;
        }

        _config = config;
        _publisher = &publisher;
        _remote = &remote;
        _subscriptions = &subscriptions;
        _server = std::make_unique<WiFiServer>(config.Port);
        _server->begin();
        _server->setNoDelay(true);

        if (!StartWorker("ESPressioStateTCP", config.Worker)) {
            _server->end();
            _server.reset();
            _publisher = nullptr;
            _remote = nullptr;
            _subscriptions = nullptr;
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
        _publisher = nullptr;
        _remote = nullptr;
        _subscriptions = nullptr;
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

protected:
    void OnWorkerIteration() override {
        std::lock_guard<std::mutex> lock(_clientsMutex);
        AcceptClientLocked();
        std::array<uint8_t, 512> buffer{};

        for (std::size_t index = 0; index < _config.MaximumClients; ++index) {
            auto& state = _clients[index];
            if (!state.Active) continue;
            if (!state.Client.connected()) {
                ResetClient(state);
                continue;
            }

            while (state.Client.available() > 0) {
                const int count = state.Client.read(buffer.data(), buffer.size());
                if (count <= 0) break;
                if (!state.StateSession.Feed(
                        buffer.data(),
                        static_cast<std::size_t>(count))) {
                    ResetClient(state);
                    break;
                }
            }
        }
    }

private:
/**
 * ESPressio Memory Audit
 * Members:
 * - Client (WiFiClient): sizeof(WiFiClient) (target/toolchain dependent) [0 bytes dynamic allocation]
 * - StateSession (Session): 145 bytes known/aligned storage + sizeof(State::StatePublishedObserverPack<SocketStateSession<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>, TContract>) (target/toolchain dependent) [_subscribers: _subscribers: Capacity * (17 bytes known/aligned storage + TContract::StateCount * (1 bytes)) element storage; _subscribers: _mutex: native synchronization state may allocate platform resources lazily; _subscribers: _observerMutex: native synchronization state may allocate platform resources lazily; _subscribers: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _decoder: _buffer: Capacity * (1 bytes) element storage; _publisherHandle: owned object: 4 bytes; _subscriptionHandle: owned object: 4 bytes; _mutex: native synchronization state may allocate platform resources lazily]
 * - ID (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * - Active (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 9 bytes known/aligned storage + sizeof(WiFiClient) (target/toolchain dependent) + 145 bytes known/aligned storage + sizeof(State::StatePublishedObserverPack<SocketStateSession<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>, TContract>) (target/toolchain dependent) [StateSession: _subscribers: _subscribers: Capacity * (17 bytes known/aligned storage + TContract::StateCount * (1 bytes)) element storage; StateSession: _subscribers: _mutex: native synchronization state may allocate platform resources lazily; StateSession: _subscribers: _observerMutex: native synchronization state may allocate platform resources lazily; StateSession: _subscribers: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; StateSession: _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; StateSession: _decoder: _buffer: Capacity * (1 bytes) element storage; StateSession: _publisherHandle: owned object: 4 bytes; StateSession: _subscriptionHandle: owned object: 4 bytes; StateSession: _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct ClientState final {
        WiFiClient Client;
        Session StateSession;
        uint64_t ID = 0;
        bool Active = false;
    };

    std::unique_ptr<WiFiServer> _server;
    std::array<ClientState, ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS> _clients;
    Config _config{};
    Publisher* _publisher = nullptr;
    RemoteManager* _remote = nullptr;
    Subscriptions* _subscriptions = nullptr;
    mutable std::mutex _clientsMutex;
    uint64_t _nextSessionID = 1;
    bool _initialized = false;

    static void ResetClient(ClientState& state) {
        state.StateSession.Shutdown();
        state.Client.stop();
        state.ID = 0;
        state.Active = false;
    }

    void AcceptClientLocked() {
        if (!_server || _publisher == nullptr || _remote == nullptr || _subscriptions == nullptr) return;
        WiFiClient incoming = _server->available();
        if (!incoming) return;

        for (std::size_t index = 0; index < _config.MaximumClients; ++index) {
            auto& state = _clients[index];
            if (state.Active && state.Client.connected()) continue;

            ResetClient(state);
            state.Client = incoming;
            state.ID = _nextSessionID++;
            if (_nextSessionID == 0) _nextSessionID = 1;
            ClientState* clientState = &state;

            SocketStateSessionConfig sessionConfig = _config.Session;
            sessionConfig.ExpectedRemoteDevice = {};
            const bool initialized = state.StateSession.Initialize(
                *_publisher,
                *_remote,
                *_subscriptions,
                sessionConfig,
                [clientState](const uint8_t* data, std::size_t size) {
                    if (!clientState->Client.connected()) return false;
                    return WriteAll(clientState->Client, data, size);
                }
            );

            if (!initialized) {
                ResetClient(state);
                return;
            }
            state.Active = true;
            return;
        }

        incoming.stop();
    }
};

} // namespace ESPressio::Sockets
