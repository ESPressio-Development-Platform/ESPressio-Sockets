#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <vector>

#include <WiFiClient.h>
#include <WiFiServer.h>

#include "../ESPressio_SocketEventFrame.hpp"
#include "../ESPressio_SocketStreamHelpers.hpp"
#include "../ESPressio_SocketWorker.hpp"
#include "ESPressio_SocketClockSynchronizationProtocol.hpp"

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 16 bytes [0 bytes dynamic allocation]
 * Members:
 * - Host (String): 12 bytes [Capacity + 1 bytes (Arduino String backing buffer when allocated)]
 * - Port (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * - ReconnectIntervalMilliseconds (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - Worker (SocketWorkerConfig): 12 bytes [0 bytes dynamic allocation]
 * Total Memory: 48 bytes [Host: Capacity + 1 bytes (Arduino String backing buffer when allocated)]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct TCPClockSynchronizationClientConfig :
    public SocketClockSynchronizationConfig {
    String Host;
    uint16_t Port = 45110;
    uint32_t ReconnectIntervalMilliseconds = 2000;
    SocketWorkerConfig Worker;
};

/**
 * ESPressio Memory Audit
 * Members:
 * - Port (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * - MaximumClients (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * - Worker (SocketWorkerConfig): 12 bytes [0 bytes dynamic allocation]
 * Total Memory: 20 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
struct TCPClockSynchronizationServerConfig {
    uint16_t Port = 45110;
    std::size_t MaximumClients = ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS;
    SocketWorkerConfig Worker;
};


/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(SocketWorkerConfig) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - _client (WiFiClient): sizeof(WiFiClient) [0 bytes dynamic allocation]
 * - _config (TCPClockSynchronizationClientConfig): 48 bytes [Host: Capacity + 1 bytes (Arduino String backing buffer when allocated)]
 * - _protocol (SocketClockSynchronizationProtocol): sizeof(SocketClockSynchronizationConfig) [0 bytes dynamic allocation]
 * - _decoder (SocketEventFrameDecoder): 12 bytes [_buffer: Capacity * 1 bytes]
 * - _lastConnectAttempt (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - _lastRequest (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - _initialized (bool): 1 bytes [0 bytes dynamic allocation]
 * - _mutex (std::mutex): sizeof(std::mutex) [0 bytes dynamic allocation]
 * Total Memory: sizeof(SocketWorkerConfig) + 4 bytes vptr + 69 bytes known members + sizeof(WiFiClient) + sizeof(SocketClockSynchronizationConfig) + sizeof(std::mutex) [_config: Host: Capacity + 1 bytes (Arduino String backing buffer when allocated); _decoder: _buffer: Capacity * 1 bytes]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class TCPClockSynchronizationClient final :
    private SocketWorker {

private:
    WiFiClient _client;
    TCPClockSynchronizationClientConfig _config;
    SocketClockSynchronizationProtocol _protocol;
    SocketEventFrameDecoder _decoder;
    uint32_t _lastConnectAttempt = 0;
    uint32_t _lastRequest = 0;
    bool _initialized = false;
    std::mutex _mutex;

    bool EnsureConnectedLocked() {
        if (_client.connected()) {
            return true;
        }

        const uint32_t now = millis();
        if (
            now - _lastConnectAttempt <
            _config.ReconnectIntervalMilliseconds
        ) {
            return false;
        }

        _lastConnectAttempt = now;
        _client.stop();
        _decoder.Reset();
        _protocol.CancelPendingRequest();

        return _client.connect(
            _config.Host.c_str(),
            _config.Port
        );
    }

    bool SendMessageLocked(
        const uint8_t* data,
        std::size_t size
    ) {
        const auto frame = BuildSocketEventFrame(data, size);
        return !frame.empty() && WriteAll(_client, frame.data(), frame.size());
    }

    void ProcessMessage(
        const std::vector<uint8_t>& message,
        uint64_t receiveTime
    ) {
        if (
            message.size() < sizeof(SocketClockSynchronizationProtocol::MessageHeader)
        ) {
            return;
        }

        SocketClockSynchronizationProtocol::MessageHeader header;
        std::memcpy(&header, message.data(), sizeof(header));

        if (
            header.Magic != SocketClockSynchronizationProtocol::MessageHeader::MagicValue ||
            header.Version != 1
        ) {
            return;
        }

        if (
            header.Type == static_cast<uint8_t>(
                SocketClockSynchronizationProtocol::MessageType::Response
            )
        ) {
            _protocol.ProcessResponse(
                message.data(),
                message.size(),
                receiveTime
            );
        } else if (
            header.Type == static_cast<uint8_t>(
                SocketClockSynchronizationProtocol::MessageType::AuthoritativeBroadcast
            )
        ) {
            _protocol.ProcessAuthoritativeBroadcast(
                message.data(),
                message.size(),
                receiveTime
            );
        }
    }

    void OnWorkerIteration() override {
        std::vector<std::vector<uint8_t>> completed;

        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (!EnsureConnectedLocked()) {
                return;
            }

            std::array<uint8_t, 256> buffer{};

            while (_client.available()) {
                const int count = _client.read(buffer.data(), buffer.size());
                if (count <= 0) {
                    break;
                }

                const bool valid = _decoder.Push(
                    buffer.data(),
                    static_cast<std::size_t>(count),
                    [&](const uint8_t* data, std::size_t size) {
                        completed.emplace_back(data, data + size);
                    }
                );

                if (!valid) {
                    _client.stop();
                    _protocol.CancelPendingRequest();
                    break;
                }
            }
        }

        for (const auto& message : completed) {
            const uint64_t receiveTime =
                _protocol.GetLocalTimestamp();
            ProcessMessage(message, receiveTime);
        }

        const uint32_t now = millis();
        if (
            _config.SynchronizationIntervalMilliseconds > 0 &&
            (
                _lastRequest == 0 ||
                now - _lastRequest >= _config.SynchronizationIntervalMilliseconds
            )
        ) {
            RequestSynchronization();
        }
    }

public:
    explicit TCPClockSynchronizationClient(
        Timing::IClockSynchronizationTarget<Timing::ClockTick>* target = nullptr
    ) : _protocol(target) {
    }

    ~TCPClockSynchronizationClient() {
        Shutdown();
    }

    bool Initialize(const TCPClockSynchronizationClientConfig& config) {
        if (_initialized) {
            return true;
        }

        if (config.Host.length() == 0 || config.Port == 0) {
            return false;
        }

        _config = config;
        _config.Mode = SocketClockSynchronizationMode::Client;
        _protocol.Configure(_config);

        if (!StartWorker("ESPressioTCPSyncC", config.Worker)) {
            return false;
        }

        _initialized = true;
        return true;
    }

    void Shutdown() {
        if (!_initialized) {
            return;
        }

        StopWorker();
        std::lock_guard<std::mutex> lock(_mutex);
        _client.stop();
        _decoder.Reset();
        _protocol.CancelPendingRequest();
        _initialized = false;
    }

    bool RequestSynchronization() {
        if (!_initialized) {
            return false;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        if (!EnsureConnectedLocked()) {
            return false;
        }

        SocketClockSynchronizationProtocol::RequestMessage request;
        if (!_protocol.BuildRequest(request)) {
            return false;
        }

        const bool sent = SendMessageLocked(
            reinterpret_cast<const uint8_t*>(&request),
            sizeof(request)
        );

        if (sent) {
            _lastRequest = millis();
        } else {
            _protocol.CancelPendingRequest();
            _client.stop();
        }

        return sent;
    }

    bool GetIsConnected() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _client.connected();
    }

    Timing::ClockSynchronizationStatus<Timing::ClockTick>
    GetSynchronizationStatus() const {
        return _protocol.GetSynchronizationStatus();
    }
};


/**
 * ESPressio Memory Audit
 * Inherited Memory Total: sizeof(SocketWorkerConfig) + 4 bytes vptr [0 bytes dynamic allocation]
 * Members:
 * - _server (std::unique_ptr<WiFiServer>): 4 bytes [owned object: sizeof(WiFiServer)]
 * - _clients (std::array<ClientState, ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS>): ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS * 13 bytes known members + sizeof(WiFiClient) [elements may add: Decoder: _buffer: Capacity * 1 bytes]
 * - _config (TCPClockSynchronizationServerConfig): 20 bytes [0 bytes dynamic allocation]
 * - _protocol (SocketClockSynchronizationProtocol): sizeof(SocketClockSynchronizationConfig) [0 bytes dynamic allocation]
 * - _initialized (bool): 1 bytes [0 bytes dynamic allocation]
 * - _mutex (std::mutex): sizeof(std::mutex) [0 bytes dynamic allocation]
 * Total Memory: sizeof(SocketWorkerConfig) + 4 bytes vptr + 25 bytes known members + ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS * 13 bytes known members + sizeof(WiFiClient) + sizeof(SocketClockSynchronizationConfig) + sizeof(std::mutex) [_server: owned object: sizeof(WiFiServer); _clients: elements may add: Decoder: _buffer: Capacity * 1 bytes]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class TCPClockSynchronizationServer final :
    private SocketWorker {

private:
/**
 * ESPressio Memory Audit
 * Members:
 * - Client (WiFiClient): sizeof(WiFiClient) [0 bytes dynamic allocation]
 * - Decoder (SocketEventFrameDecoder): 12 bytes [_buffer: Capacity * 1 bytes]
 * - Active (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 13 bytes known members + sizeof(WiFiClient) [Decoder: _buffer: Capacity * 1 bytes]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct ClientState {
        WiFiClient Client;
        SocketEventFrameDecoder Decoder;
        bool Active = false;
    };

    std::unique_ptr<WiFiServer> _server;
    std::array<ClientState, ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS> _clients;
    TCPClockSynchronizationServerConfig _config;
    SocketClockSynchronizationProtocol _protocol;
    bool _initialized = false;
    std::mutex _mutex;

    bool SendMessage(
        WiFiClient& client,
        const uint8_t* data,
        std::size_t size
    ) {
        const auto frame = BuildSocketEventFrame(data, size);
        return !frame.empty() && WriteAll(client, frame.data(), frame.size());
    }

    void AcceptClientLocked() {
        if (_server == nullptr) {
            return;
        }

        WiFiClient incoming = _server->available();
        if (!incoming) {
            return;
        }

        const std::size_t limit = std::min(_config.MaximumClients, _clients.size());
        for (std::size_t i = 0; i < limit; ++i) {
            if (!_clients[i].Active || !_clients[i].Client.connected()) {
                _clients[i].Client.stop();
                _clients[i].Client = incoming;
                _clients[i].Decoder.Reset();
                _clients[i].Active = true;
                return;
            }
        }

        incoming.stop();
    }

    void OnWorkerIteration() override {
        std::lock_guard<std::mutex> lock(_mutex);
        AcceptClientLocked();

        const std::size_t limit = std::min(_config.MaximumClients, _clients.size());
        std::array<uint8_t, 256> buffer{};

        for (std::size_t i = 0; i < limit; ++i) {
            auto& state = _clients[i];
            if (!state.Active) {
                continue;
            }

            if (!state.Client.connected()) {
                state.Client.stop();
                state.Decoder.Reset();
                state.Active = false;
                continue;
            }

            while (state.Client.available()) {
                const int count = state.Client.read(buffer.data(), buffer.size());
                if (count <= 0) {
                    break;
                }

                const uint64_t receiveTime =
                    _protocol.GetLocalTimestamp();

                const bool valid = state.Decoder.Push(
                    buffer.data(),
                    static_cast<std::size_t>(count),
                    [&](const uint8_t* data, std::size_t size) {
                        _protocol.ProcessRequest(
                            data,
                            size,
                            receiveTime,
                            [&](const uint8_t* response, std::size_t responseSize) {
                                return SendMessage(state.Client, response, responseSize);
                            }
                        );
                    }
                );

                if (!valid) {
                    state.Client.stop();
                    state.Active = false;
                    break;
                }
            }
        }
    }

public:
    explicit TCPClockSynchronizationServer(
        Timing::IClockSynchronizationTarget<Timing::ClockTick>* target = nullptr
    ) : _protocol(target) {
        SocketClockSynchronizationConfig config;
        config.Mode = SocketClockSynchronizationMode::Reference;
        _protocol.Configure(config);
    }

    ~TCPClockSynchronizationServer() {
        Shutdown();
    }

    bool Initialize(const TCPClockSynchronizationServerConfig& config) {
        if (_initialized) {
            return true;
        }

        if (config.Port == 0 || config.MaximumClients == 0) {
            return false;
        }

        _config = config;
        _server = std::make_unique<WiFiServer>(config.Port);
        _server->begin();
        _server->setNoDelay(true);

        if (!StartWorker("ESPressioTCPSyncS", config.Worker)) {
            _server->end();
            _server.reset();
            return false;
        }

        _initialized = true;
        return true;
    }

    void Shutdown() {
        if (!_initialized) {
            return;
        }

        StopWorker();
        std::lock_guard<std::mutex> lock(_mutex);

        for (auto& state : _clients) {
            state.Client.stop();
            state.Decoder.Reset();
            state.Active = false;
        }

        if (_server != nullptr) {
            _server->end();
            _server.reset();
        }

        _initialized = false;
    }
};

}
