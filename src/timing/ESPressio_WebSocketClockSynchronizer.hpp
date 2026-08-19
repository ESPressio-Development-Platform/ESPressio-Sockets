#pragma once

#if !__has_include(<WebSocketsClient.h>) || !__has_include(<WebSocketsServer.h>)
#error "WebSocket Clock Synchronization requires Links2004 arduinoWebSockets."
#endif

#include <mutex>

#include <WebSocketsClient.h>
#include <WebSocketsServer.h>

#include "../ESPressio_SocketWorker.hpp"
#include "ESPressio_SocketClockSynchronizationProtocol.hpp"

namespace ESPressio::Sockets {

struct WebSocketClockSynchronizationClientConfig :
    public SocketClockSynchronizationConfig {
    String Host;
    uint16_t Port = 45120;
    String Path = "/";
    String Protocol = "espressio-clock";
    bool Secure = false;
    const char* CACertificate = nullptr;
    uint32_t ReconnectIntervalMilliseconds = 2000;
    bool EnableHeartbeat = true;
    uint32_t PingIntervalMilliseconds = 15000;
    uint32_t PongTimeoutMilliseconds = 3000;
    uint8_t DisconnectTimeoutCount = 2;
    SocketWorkerConfig Worker;
};

struct WebSocketClockSynchronizationServerConfig {
    uint16_t Port = 45120;
    String Origin;
    String Protocol = "espressio-clock";
    bool EnableHeartbeat = true;
    uint32_t PingIntervalMilliseconds = 15000;
    uint32_t PongTimeoutMilliseconds = 3000;
    uint8_t DisconnectTimeoutCount = 2;
    SocketWorkerConfig Worker;
};


class WebSocketClockSynchronizationClient final :
    private SocketWorker {

private:
    WebSocketsClient _webSocket;
    WebSocketClockSynchronizationClientConfig _config;
    SocketClockSynchronizationProtocol _protocol;
    std::recursive_mutex _mutex;
    uint32_t _lastRequest = 0;
    bool _initialized = false;

    void HandleEvent(
        WStype_t type,
        uint8_t* payload,
        std::size_t length
    ) {
        if (type != WStype_BIN || payload == nullptr || length == 0) {
            return;
        }

        const uint64_t receiveTime =
            _protocol.GetLocalTimestamp();

        if (length < sizeof(SocketClockSynchronizationProtocol::MessageHeader)) {
            return;
        }

        SocketClockSynchronizationProtocol::MessageHeader header;
        std::memcpy(&header, payload, sizeof(header));

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
            _protocol.ProcessResponse(payload, length, receiveTime);
        } else if (
            header.Type == static_cast<uint8_t>(
                SocketClockSynchronizationProtocol::MessageType::AuthoritativeBroadcast
            )
        ) {
            _protocol.ProcessAuthoritativeBroadcast(payload, length, receiveTime);
        }
    }

    void OnWorkerIteration() override {
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            _webSocket.loop();
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
    explicit WebSocketClockSynchronizationClient(
        Timing::IClockSynchronizationTarget<Timing::ClockTick>* target = nullptr
    ) : _protocol(target) {
    }

    ~WebSocketClockSynchronizationClient() {
        Shutdown();
    }

    bool Initialize(const WebSocketClockSynchronizationClientConfig& config) {
        if (_initialized) {
            return true;
        }

        if (config.Host.length() == 0 || config.Port == 0) {
            return false;
        }

        _config = config;
        _config.Mode = SocketClockSynchronizationMode::Client;
        _protocol.Configure(_config);

        _webSocket.onEvent(
            [this](WStype_t type, uint8_t* payload, std::size_t length) {
                HandleEvent(type, payload, length);
            }
        );

        _webSocket.setReconnectInterval(config.ReconnectIntervalMilliseconds);

        if (config.EnableHeartbeat) {
            _webSocket.enableHeartbeat(
                config.PingIntervalMilliseconds,
                config.PongTimeoutMilliseconds,
                config.DisconnectTimeoutCount
            );
        }

        if (config.Secure) {
            _webSocket.beginSslWithCA(
                config.Host.c_str(),
                config.Port,
                config.Path.c_str(),
                config.CACertificate,
                config.Protocol.c_str()
            );
        } else {
            _webSocket.begin(
                config.Host.c_str(),
                config.Port,
                config.Path.c_str(),
                config.Protocol.c_str()
            );
        }

        if (!StartWorker("ESPressioWSSyncC", config.Worker)) {
            _webSocket.disconnect();
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
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _webSocket.disconnect();
        _protocol.CancelPendingRequest();
        _initialized = false;
    }

    bool RequestSynchronization() {
        if (!_initialized) {
            return false;
        }

        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (!_webSocket.isConnected()) {
            return false;
        }

        SocketClockSynchronizationProtocol::RequestMessage request;
        if (!_protocol.BuildRequest(request)) {
            return false;
        }

        const bool sent = _webSocket.sendBIN(
            reinterpret_cast<uint8_t*>(&request),
            sizeof(request)
        );

        if (sent) {
            _lastRequest = millis();
        } else {
            _protocol.CancelPendingRequest();
        }

        return sent;
    }

    Timing::ClockSynchronizationStatus<Timing::ClockTick>
    GetSynchronizationStatus() const {
        return _protocol.GetSynchronizationStatus();
    }
};


class WebSocketClockSynchronizationServer final :
    private SocketWorker {

private:
    std::unique_ptr<WebSocketsServer> _server;
    WebSocketClockSynchronizationServerConfig _config;
    SocketClockSynchronizationProtocol _protocol;
    std::recursive_mutex _mutex;
    bool _initialized = false;

    void HandleEvent(
        uint8_t client,
        WStype_t type,
        uint8_t* payload,
        std::size_t length
    ) {
        if (type != WStype_BIN || payload == nullptr || length == 0) {
            return;
        }

        const uint64_t receiveTime =
            _protocol.GetLocalTimestamp();

        _protocol.ProcessRequest(
            payload,
            length,
            receiveTime,
            [&](const uint8_t* response, std::size_t responseSize) {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                return _server != nullptr && _server->sendBIN(
                    client,
                    const_cast<uint8_t*>(response),
                    responseSize
                );
            }
        );
    }

    void OnWorkerIteration() override {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (_server != nullptr) {
            _server->loop();
        }
    }

public:
    explicit WebSocketClockSynchronizationServer(
        Timing::IClockSynchronizationTarget<Timing::ClockTick>* target = nullptr
    ) : _protocol(target) {
        SocketClockSynchronizationConfig protocolConfig;
        protocolConfig.Mode = SocketClockSynchronizationMode::Reference;
        _protocol.Configure(protocolConfig);
    }

    ~WebSocketClockSynchronizationServer() {
        Shutdown();
    }

    bool Initialize(const WebSocketClockSynchronizationServerConfig& config) {
        if (_initialized) {
            return true;
        }

        if (config.Port == 0) {
            return false;
        }

        _config = config;
        _server = std::make_unique<WebSocketsServer>(
            config.Port,
            config.Origin,
            config.Protocol
        );

        _server->onEvent(
            [this](
                uint8_t client,
                WStype_t type,
                uint8_t* payload,
                std::size_t length
            ) {
                HandleEvent(client, type, payload, length);
            }
        );

        if (config.EnableHeartbeat) {
            _server->enableHeartbeat(
                config.PingIntervalMilliseconds,
                config.PongTimeoutMilliseconds,
                config.DisconnectTimeoutCount
            );
        }

        _server->begin();

        if (!StartWorker("ESPressioWSSyncS", config.Worker)) {
            _server->close();
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
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (_server != nullptr) {
            _server->close();
            _server.reset();
        }
        _initialized = false;
    }
};

}
