#pragma once

#if !__has_include(<ESPressio_State.hpp>)
#error "SocketStateSession requires ESPressio State. Add the active ESPressio-State dependency when using this optional integration."
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <tuple>
#include <utility>
#include <vector>

#include <ESPressio_State.hpp>

#include "ESPressio_SocketStateFrame.hpp"

namespace ESPressio::Sockets {

struct SocketStateSessionConfig final {
    std::size_t MaximumProtocolMessageBytes = 4096;
    State::DeviceIdentifier ExpectedRemoteDevice{};
    bool SendDisconnectOnShutdown = true;
};

using SocketStateSendHandler = std::function<bool(const uint8_t*, std::size_t)>;
using SocketStatePeerBoundHandler = std::function<void(const State::DeviceIdentifier&)>;
using SocketStateAcknowledgementHandler = std::function<void(const State::StateAcknowledgement&)>;

template<
    typename TContract,
    std::size_t TMaximumRemoteDevices,
    std::size_t TSubscriptionCapacity
>
class SocketStateSession final :
    public State::IStatePublisherObserver,
    public State::StatePublishedObserverPack<
        SocketStateSession<TContract, TMaximumRemoteDevices, TSubscriptionCapacity>,
        TContract
    >,
    public State::IStateSubscriptionRegistryObserver {
public:
    using Publisher = State::StatePublisher<TContract>;
    using RemoteManager = State::RemoteStateManager<TContract, TMaximumRemoteDevices>;
    using Subscriptions = State::StateSubscriptionRegistry<TSubscriptionCapacity>;

    SocketStateSession() = default;
    SocketStateSession(const SocketStateSession&) = delete;
    SocketStateSession& operator=(const SocketStateSession&) = delete;
    ~SocketStateSession() { Shutdown(); }

    bool Initialize(
        Publisher& publisher,
        RemoteManager& remote,
        Subscriptions& subscriptions,
        SocketStateSessionConfig config,
        SocketStateSendHandler sendHandler
    ) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (_initialized) return true;
        if (!sendHandler || config.MaximumProtocolMessageBytes < State::StateProtocol::ControlSize) {
            return false;
        }

        _publisher = &publisher;
        _remote = &remote;
        _subscriptions = &subscriptions;
        _config = config;
        _sendHandler = std::move(sendHandler);
        _decoder.SetMaximumPayloadBytes(config.MaximumProtocolMessageBytes);
        _decoder.Reset();

        _publisherHandle = _publisher->RegisterContractObserver(this);
        _subscriptionHandle = _subscriptions->RegisterObserver(this);
        if (!_publisherHandle || !_subscriptionHandle) {
            _publisherHandle.reset();
            _subscriptionHandle.reset();
            _publisher = nullptr;
            _remote = nullptr;
            _subscriptions = nullptr;
            _sendHandler = {};
            return false;
        }

        _initialized = true;
        if (!config.ExpectedRemoteDevice.IsZero()) {
            if (!BindPeerLocked(config.ExpectedRemoteDevice)) {
                ShutdownLocked(false);
                return false;
            }
        }
        SendCurrentSubscriptionsLocked();
        return true;
    }

    void Shutdown() {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        ShutdownLocked(true);
    }

    bool Feed(const uint8_t* data, std::size_t size) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (!_initialized) return false;
        bool allAccepted = true;
        const bool framingValid = _decoder.Push(
            data,
            size,
            [&](const uint8_t* payload, std::size_t payloadSize) {
                if (!HandleProtocolMessageLocked(payload, payloadSize)) {
                    allAccepted = false;
                }
            }
        );
        return framingValid && allAccepted;
    }

    bool GetIsInitialized() const {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        return _initialized;
    }

    bool HasRemoteDevice() const {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        return !_remoteDevice.IsZero();
    }

    State::DeviceIdentifier RemoteDevice() const {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        return _remoteDevice;
    }

    void SetPeerBoundHandler(SocketStatePeerBoundHandler handler) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _peerBoundHandler = std::move(handler);
    }

    void SetAcknowledgementHandler(SocketStateAcknowledgementHandler handler) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _acknowledgementHandler = std::move(handler);
    }

    bool RequestResynchronization(State::StateTypeId typeId = 0) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (!_initialized) return false;
        return SendControlLocked(State::StateProtocol::MessageType::Resynchronize, typeId);
    }

    template<typename TDefinition>
    bool RequestResynchronization() {
        static_assert(TContract::template Contains<TDefinition>, "State definition is not part of this StateContract");
        return RequestResynchronization(State::StateTypeIdOf<TDefinition>);
    }

    template<typename TDefinition>
    void OnTypedStatePublished(
        const State::StateUpdate<State::StateValueType<TDefinition>>& update
    ) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (!_initialized || _remoteDevice.IsZero()) return;
        if (!_subscribers.template IsSubscribed<TDefinition>(_remoteDevice)) return;
        (void)SendUpdateLocked<TDefinition>(update);
    }

    void OnStateSubscribed(
        State::StateTypeId typeId,
        State::StateSubscriptionScope scope,
        const State::DeviceIdentifier& device
    ) override {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (!_initialized || !SubscriptionAppliesLocked(scope, device)) return;
        (void)SendControlLocked(State::StateProtocol::MessageType::Subscribe, typeId);
    }

    void OnStateUnsubscribed(
        State::StateTypeId typeId,
        State::StateSubscriptionScope scope,
        const State::DeviceIdentifier& device
    ) override {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (!_initialized || !SubscriptionAppliesLocked(scope, device)) return;
        (void)SendControlLocked(State::StateProtocol::MessageType::Unsubscribe, typeId);
    }

private:
    Publisher* _publisher = nullptr;
    RemoteManager* _remote = nullptr;
    Subscriptions* _subscriptions = nullptr;
    State::StateSubscriberRegistry<TContract, 1> _subscribers;
    SocketStateSessionConfig _config{};
    SocketStateSendHandler _sendHandler;
    SocketStatePeerBoundHandler _peerBoundHandler;
    SocketStateAcknowledgementHandler _acknowledgementHandler;
    SocketStateFrameDecoder _decoder;
    Observable::ObserverHandlePtr _publisherHandle;
    Observable::ObserverHandlePtr _subscriptionHandle;
    State::DeviceIdentifier _remoteDevice{};
    mutable std::recursive_mutex _mutex;
    bool _initialized = false;

    void ShutdownLocked(bool notifyRemote) {
        if (!_initialized && _publisher == nullptr && _remote == nullptr) return;
        if (notifyRemote && _initialized && _config.SendDisconnectOnShutdown && !_remoteDevice.IsZero()) {
            (void)SendControlLocked(State::StateProtocol::MessageType::Disconnect, 0);
        }
        if (_remote != nullptr && !_remoteDevice.IsZero()) {
            (void)_remote->SetAvailability(
                _remoteDevice,
                State::RemoteDeviceAvailability::Disconnected
            );
        }
        if (!_remoteDevice.IsZero()) {
            (void)_subscribers.Remove(_remoteDevice);
        }
        _publisherHandle.reset();
        _subscriptionHandle.reset();
        _decoder.Reset();
        _sendHandler = {};
        _peerBoundHandler = {};
        _acknowledgementHandler = {};
        _remoteDevice = {};
        _publisher = nullptr;
        _remote = nullptr;
        _subscriptions = nullptr;
        _initialized = false;
    }

    bool BindPeerLocked(const State::DeviceIdentifier& device) {
        if (device.IsZero()) return false;
        if (!_config.ExpectedRemoteDevice.IsZero() && device != _config.ExpectedRemoteDevice) {
            return false;
        }
        if (!_remoteDevice.IsZero()) return _remoteDevice == device;
        _remoteDevice = device;
        if (_remote != nullptr) {
            (void)_remote->SetAvailability(device, State::RemoteDeviceAvailability::Connected);
        }
        if (_peerBoundHandler) _peerBoundHandler(device);
        SendCurrentSubscriptionsLocked();
        return true;
    }

    bool SubscriptionAppliesLocked(
        State::StateSubscriptionScope scope,
        const State::DeviceIdentifier& device
    ) const {
        if (scope == State::StateSubscriptionScope::AnyDevice) return true;
        if (!_remoteDevice.IsZero()) return device == _remoteDevice;
        if (!_config.ExpectedRemoteDevice.IsZero()) return device == _config.ExpectedRemoteDevice;
        return false;
    }

    void SendCurrentSubscriptionsLocked() {
        if (_subscriptions == nullptr || !_initialized) return;
        _subscriptions->ForEach([&](const typename Subscriptions::Descriptor& descriptor) {
            if (SubscriptionAppliesLocked(descriptor.Scope, descriptor.Device)) {
                (void)SendControlLocked(
                    State::StateProtocol::MessageType::Subscribe,
                    descriptor.TypeId
                );
            }
        });
    }

    bool SendProtocolPayloadLocked(const uint8_t* payload, std::size_t size) {
        if (!_sendHandler || payload == nullptr || size == 0 ||
            size > _config.MaximumProtocolMessageBytes) {
            return false;
        }
        auto frame = BuildSocketStateFrame(
            payload,
            size,
            _config.MaximumProtocolMessageBytes
        );
        return !frame.empty() && _sendHandler(frame.data(), frame.size());
    }

    bool SendControlLocked(
        State::StateProtocol::MessageType type,
        State::StateTypeId typeId
    ) {
        if (_publisher == nullptr) return false;
        State::StateProtocol::ControlMessage control{
            type,
            _publisher->Origin(),
            typeId
        };
        std::array<uint8_t, State::StateProtocol::ControlSize> payload{};
        std::size_t size = 0;
        return State::StateProtocol::EncodeControl(
                   control,
                   payload.data(),
                   payload.size(),
                   size
               ) && SendProtocolPayloadLocked(payload.data(), size);
    }

    bool SendAcknowledgementLocked(const State::StateUpdateHeader& header) {
        State::StateAcknowledgement acknowledgement{
            header.Origin,
            header.TypeId,
            header.Epoch,
            header.Revision
        };
        std::array<uint8_t, State::StateProtocol::AcknowledgementSize> payload{};
        std::size_t size = 0;
        return State::StateProtocol::EncodeAcknowledgement(
                   acknowledgement,
                   payload.data(),
                   payload.size(),
                   size
               ) && SendProtocolPayloadLocked(payload.data(), size);
    }

    template<typename TDefinition>
    bool SendUpdateLocked(
        const State::StateUpdate<State::StateValueType<TDefinition>>& update
    ) {
        std::vector<uint8_t> payload(_config.MaximumProtocolMessageBytes);
        std::size_t size = 0;
        return State::StateProtocol::template EncodeUpdate<TDefinition>(
                   update,
                   payload.data(),
                   payload.size(),
                   size
               ) && SendProtocolPayloadLocked(payload.data(), size);
    }

    template<typename TDefinition>
    bool SendSnapshotLocked() {
        if (_publisher == nullptr) return false;
        State::StateUpdate<State::StateValueType<TDefinition>> update;
        return _publisher->template Snapshot<TDefinition>(update) &&
            SendUpdateLocked<TDefinition>(update);
    }

    template<std::size_t TIndex = 0>
    bool SendSnapshotByTypeLocked(State::StateTypeId typeId) {
        if constexpr (TIndex < TContract::StateCount) {
            using Definition = typename std::tuple_element<
                TIndex,
                typename TContract::Definitions
            >::type;
            if (typeId == State::StateTypeIdOf<Definition>) {
                return SendSnapshotLocked<Definition>();
            }
            return SendSnapshotByTypeLocked<TIndex + 1>(typeId);
        }
        return false;
    }

    template<std::size_t TIndex = 0>
    bool ApplyIncomingLocked(
        const State::StateProtocol::ParsedUpdate& parsed,
        bool& exactDuplicate
    ) {
        if constexpr (TIndex < TContract::StateCount) {
            using Definition = typename std::tuple_element<
                TIndex,
                typename TContract::Definitions
            >::type;
            if (parsed.Header.TypeId == State::StateTypeIdOf<Definition>) {
                State::StateValueType<Definition> value{};
                if (!State::StateProtocol::template DecodeValue<Definition>(parsed, value)) {
                    return false;
                }
                State::RemoteStateSnapshot<State::StateValueType<Definition>> current;
                if (_remote->template Read<Definition>(parsed.Header.Origin, current) && current.HasValue) {
                    exactDuplicate = current.Epoch == parsed.Header.Epoch &&
                        current.Revision == parsed.Header.Revision;
                }
                return exactDuplicate || _remote->template Apply<Definition>(
                    parsed.Header.Origin,
                    parsed.Header.Epoch,
                    parsed.Header.Revision,
                    std::move(value)
                );
            }
            return ApplyIncomingLocked<TIndex + 1>(parsed, exactDuplicate);
        }
        return false;
    }

    bool HandleProtocolMessageLocked(const uint8_t* payload, std::size_t size) {
        State::StateProtocol::MessageType type{};
        if (!State::StateProtocol::GetMessageType(payload, size, type)) return false;

        if (type == State::StateProtocol::MessageType::Update) {
            State::StateProtocol::ParsedUpdate parsed;
            if (!State::StateProtocol::DecodeUpdate(payload, size, parsed)) return false;
            if (!BindPeerLocked(parsed.Header.Origin)) return false;
            if (!_subscriptions->IsSubscribed(_remoteDevice, parsed.Header.TypeId)) return false;
            bool duplicate = false;
            if (!ApplyIncomingLocked(parsed, duplicate)) return false;
            return SendAcknowledgementLocked(parsed.Header);
        }

        if (type == State::StateProtocol::MessageType::Acknowledgement) {
            State::StateAcknowledgement acknowledgement;
            if (!State::StateProtocol::DecodeAcknowledgement(payload, size, acknowledgement)) return false;
            if (_publisher == nullptr || acknowledgement.Origin != _publisher->Origin()) return false;
            if (_acknowledgementHandler) _acknowledgementHandler(acknowledgement);
            return true;
        }

        State::StateProtocol::ControlMessage control;
        if (!State::StateProtocol::DecodeControl(payload, size, control)) return false;
        if (!BindPeerLocked(control.Device)) return false;

        switch (control.Type) {
            case State::StateProtocol::MessageType::Subscribe:
                if (!_subscribers.Subscribe(_remoteDevice, control.TypeId)) return false;
                (void)SendSnapshotByTypeLocked(control.TypeId);
                return SendControlLocked(
                    State::StateProtocol::MessageType::SubscribeAcknowledgement,
                    control.TypeId
                );

            case State::StateProtocol::MessageType::Unsubscribe:
                (void)_subscribers.Unsubscribe(_remoteDevice, control.TypeId);
                return SendControlLocked(
                    State::StateProtocol::MessageType::UnsubscribeAcknowledgement,
                    control.TypeId
                );

            case State::StateProtocol::MessageType::Resynchronize:
                if (control.TypeId != 0) {
                    if (_subscribers.IsSubscribed(_remoteDevice, control.TypeId)) {
                        return SendSnapshotByTypeLocked(control.TypeId);
                    }
                    return true;
                }
                _subscribers.ForEachSubscribedType(
                    _remoteDevice,
                    [&](State::StateTypeId subscribedType) {
                        (void)SendSnapshotByTypeLocked(subscribedType);
                    }
                );
                return true;

            case State::StateProtocol::MessageType::Disconnect:
                (void)_subscribers.Remove(_remoteDevice);
                if (_remote != nullptr) {
                    (void)_remote->SetAvailability(
                        _remoteDevice,
                        State::RemoteDeviceAvailability::Disconnected
                    );
                }
                return true;

            case State::StateProtocol::MessageType::SubscribeAcknowledgement:
            case State::StateProtocol::MessageType::UnsubscribeAcknowledgement:
                return true;

            default:
                return false;
        }
    }
};

} // namespace ESPressio::Sockets
