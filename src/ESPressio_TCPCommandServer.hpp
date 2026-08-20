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

struct TCPCommandServerConfig {
    uint16_t Port = 0;
    std::size_t MaximumClients = ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS;
    SocketCommandSessionConfig Session;
    SocketWorkerConfig Worker;
};

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
