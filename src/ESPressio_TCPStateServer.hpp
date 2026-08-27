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
