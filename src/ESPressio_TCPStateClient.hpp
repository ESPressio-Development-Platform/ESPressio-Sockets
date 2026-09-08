#pragma once

#if !__has_include(<ESPressio_State.hpp>)
#error "TCPStateClient requires ESPressio State. Add the active ESPressio-State dependency when using this optional integration."
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include <WiFiClient.h>

#include "ESPressio_SocketStateSession.hpp"
#include "ESPressio_SocketStreamHelpers.hpp"
#include "ESPressio_SocketTypes.hpp"
#include "ESPressio_SocketWorker.hpp"

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Members:
 * - Host (String): 12 bytes [Capacity + 1 bytes backing buffer when allocated]
 * - Port (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * - ReconnectIntervalMilliseconds (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - Session (SocketStateSessionConfig): 24 bytes [0 bytes dynamic allocation]
 * - Worker (SocketWorkerConfig): 16 bytes [0 bytes dynamic allocation]
 * Total Memory: 60 bytes [Host: Capacity + 1 bytes backing buffer when allocated]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
template<
    typename TContract,
    std::size_t TMaximumRemoteDevices,
    std::size_t TSubscriptionCapacity
>
struct TCPStateClientConfig final {
    String Host;
    uint16_t Port = 0;
    uint32_t ReconnectIntervalMilliseconds = 2000;
    SocketStateSessionConfig Session;
    SocketWorkerConfig Worker;
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 36 bytes [SocketWorker: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Members:
 * - _client (WiFiClient): sizeof(WiFiClient) (target/toolchain dependent) [0 bytes dynamic allocation]
 * - _session (Session): 145 bytes known/aligned storage + sizeof(State::StatePublishedObserverPack<SocketStateSession<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>, TContract>) (target/toolchain dependent) [_subscribers: _subscribers: Capacity * (17 bytes known/aligned storage + TContract::StateCount * (1 bytes)) element storage; _subscribers: _mutex: native synchronization state may allocate platform resources lazily; _subscribers: _observerMutex: native synchronization state may allocate platform resources lazily; _subscribers: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _decoder: _buffer: Capacity * (1 bytes) element storage; _publisherHandle: owned object: 4 bytes; _subscriptionHandle: owned object: 4 bytes; _mutex: native synchronization state may allocate platform resources lazily]
 * - _config (Config): 1 bytes [0 bytes dynamic allocation]
 * - _publisher (Publisher*): 4 bytes [0 bytes dynamic allocation]
 * - _remote (RemoteManager*): 4 bytes [0 bytes dynamic allocation]
 * - _subscriptions (Subscriptions*): 4 bytes [0 bytes dynamic allocation]
 * - _clientMutex (std::mutex): 4 bytes [native synchronization state may allocate platform resources lazily]
 * - _lastConnectAttempt (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - _initialized (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 61 bytes known/aligned storage + sizeof(WiFiClient) (target/toolchain dependent) + 145 bytes known/aligned storage + sizeof(State::StatePublishedObserverPack<SocketStateSession<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>, TContract>) (target/toolchain dependent) [SocketWorker: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; SocketWorker: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _session: _subscribers: _subscribers: Capacity * (17 bytes known/aligned storage + TContract::StateCount * (1 bytes)) element storage; _session: _subscribers: _mutex: native synchronization state may allocate platform resources lazily; _session: _subscribers: _observerMutex: native synchronization state may allocate platform resources lazily; _session: _subscribers: _observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; _session: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; _session: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; _session: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; _session: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; _session: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; _session: _subscribers: _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; _session: _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; _session: _subscribers: _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _session: _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; _session: _subscribers: _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _session: _decoder: _buffer: Capacity * (1 bytes) element storage; _session: _publisherHandle: owned object: 4 bytes; _session: _subscriptionHandle: owned object: 4 bytes; _session: _mutex: native synchronization state may allocate platform resources lazily; _clientMutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
template<
    typename TContract,
    std::size_t TMaximumRemoteDevices,
    std::size_t TSubscriptionCapacity
>
class TCPStateClient final : private SocketWorker {
public:
    using Publisher = State::StatePublisher<TContract>;
    using RemoteManager = State::RemoteStateManager<TContract, TMaximumRemoteDevices>;
    using Subscriptions = State::StateSubscriptionRegistry<TSubscriptionCapacity>;
    using Session = SocketStateSession<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>;
    using Config = TCPStateClientConfig<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>;

    TCPStateClient() = default;
    TCPStateClient(const TCPStateClient&) = delete;
    TCPStateClient& operator=(const TCPStateClient&) = delete;
    ~TCPStateClient() override { Shutdown(); }

    bool Initialize(
        const Config& config,
        Publisher& publisher,
        RemoteManager& remote,
        Subscriptions& subscriptions
    ) {
        if (_initialized) return true;
        if (config.Host.length() == 0 || config.Port == 0 ||
            config.Session.MaximumProtocolMessageBytes == 0) {
            return false;
        }

        _config = config;
        _publisher = &publisher;
        _remote = &remote;
        _subscriptions = &subscriptions;
        _lastConnectAttempt = 0;

        if (!StartWorker("ESPressioStateC", config.Worker)) {
            _publisher = nullptr;
            _remote = nullptr;
            _subscriptions = nullptr;
            return false;
        }
        _initialized = true;
        return true;
    }

    void Shutdown() {
        if (!_initialized) return;
        StopWorker();
        std::lock_guard<std::mutex> lock(_clientMutex);
        DisconnectLocked();
        _publisher = nullptr;
        _remote = nullptr;
        _subscriptions = nullptr;
        _initialized = false;
    }

    bool GetIsConnected() const {
        std::lock_guard<std::mutex> lock(_clientMutex);
        return _client.connected() && _session.GetIsInitialized();
    }

    bool HasRemoteDevice() const {
        std::lock_guard<std::mutex> lock(_clientMutex);
        return _session.HasRemoteDevice();
    }

    State::DeviceIdentifier RemoteDevice() const {
        std::lock_guard<std::mutex> lock(_clientMutex);
        return _session.RemoteDevice();
    }

    bool RequestResynchronization(State::StateTypeId typeId = 0) {
        std::lock_guard<std::mutex> lock(_clientMutex);
        return _session.RequestResynchronization(typeId);
    }

protected:
    void OnWorkerIteration() override {
        std::lock_guard<std::mutex> lock(_clientMutex);
        if (!EnsureConnectedLocked()) return;

        std::array<uint8_t, 512> buffer{};
        while (_client.available() > 0) {
            const int count = _client.read(buffer.data(), buffer.size());
            if (count <= 0) break;
            if (!_session.Feed(buffer.data(), static_cast<std::size_t>(count))) {
                DisconnectLocked();
                break;
            }
        }

        if (!_client.connected()) DisconnectLocked();
    }

private:
    WiFiClient _client;
    Session _session;
    Config _config{};
    Publisher* _publisher = nullptr;
    RemoteManager* _remote = nullptr;
    Subscriptions* _subscriptions = nullptr;
    mutable std::mutex _clientMutex;
    uint32_t _lastConnectAttempt = 0;
    bool _initialized = false;

    void DisconnectLocked() {
        _session.Shutdown();
        _client.stop();
    }

    bool EnsureConnectedLocked() {
        if (_client.connected() && _session.GetIsInitialized()) return true;
        if (_publisher == nullptr || _remote == nullptr || _subscriptions == nullptr) return false;

        const uint32_t now = millis();
        if (now - _lastConnectAttempt < _config.ReconnectIntervalMilliseconds) return false;
        _lastConnectAttempt = now;

        DisconnectLocked();
        if (!_client.connect(_config.Host.c_str(), _config.Port)) return false;
        _client.setNoDelay(true);

        const bool initialized = _session.Initialize(
            *_publisher,
            *_remote,
            *_subscriptions,
            _config.Session,
            [this](const uint8_t* data, std::size_t size) {
                if (!_client.connected()) return false;
                return WriteAll(_client, data, size);
            }
        );
        if (!initialized) {
            _client.stop();
            return false;
        }
        return true;
    }
};

} // namespace ESPressio::Sockets
