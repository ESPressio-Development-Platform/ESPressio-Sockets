#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>

#include <WiFiUdp.h>

#include "../ESPressio_SocketWorker.hpp"
#include "ESPressio_SocketClockSynchronizationProtocol.hpp"
#include "ESPressio_UDPClockSynchronizationTypes.hpp"

namespace ESPressio::Sockets {

class UDPClockSynchronizer final :
    private SocketWorker {

private:
    WiFiUDP _udp;
    UDPClockSynchronizationConfig _config;
    SocketClockSynchronizationProtocol _protocol;

    uint64_t _lastRequestMilliseconds = 0;
    uint64_t _lastBroadcastMilliseconds = 0;
    bool _initialized = false;
    mutable std::mutex _mutex;

    bool SendDatagram(
        const IPAddress& address,
        uint16_t port,
        const uint8_t* data,
        std::size_t size
    ) {
        if (
            port == 0 ||
            static_cast<uint32_t>(address) == 0 ||
            data == nullptr ||
            size == 0
        ) {
            return false;
        }

        return
            _udp.beginPacket(address, port) &&
            _udp.write(data, size) == size &&
            _udp.endPacket();
    }

    void HandlePacket(
        const IPAddress& source,
        uint16_t sourcePort,
        const uint8_t* data,
        std::size_t size,
        uint64_t localReceiveTime
    ) {
        if (
            data == nullptr ||
            size < sizeof(SocketClockSynchronizationProtocol::MessageHeader)
        ) {
            return;
        }

        SocketClockSynchronizationProtocol::MessageHeader header;
        std::memcpy(&header, data, sizeof(header));

        if (
            header.Magic != SocketClockSynchronizationProtocol::MessageHeader::MagicValue ||
            header.Version != 1
        ) {
            return;
        }

        const auto type = static_cast<SocketClockSynchronizationProtocol::MessageType>(
            header.Type
        );

        if (type == SocketClockSynchronizationProtocol::MessageType::Request) {
            _protocol.ProcessRequest(
                data,
                size,
                localReceiveTime,
                [&](const uint8_t* response, std::size_t responseSize) {
                    return SendDatagram(
                        source,
                        sourcePort,
                        response,
                        responseSize
                    );
                }
            );
        } else if (type == SocketClockSynchronizationProtocol::MessageType::Response) {
            if (
                static_cast<uint32_t>(_config.ReferenceAddress) != 0 &&
                source != _config.ReferenceAddress
            ) {
                return;
            }

            _protocol.ProcessResponse(
                data,
                size,
                localReceiveTime
            );
        } else if (type == SocketClockSynchronizationProtocol::MessageType::AuthoritativeBroadcast) {
            _protocol.ProcessAuthoritativeBroadcast(
                data,
                size,
                localReceiveTime
            );
        }
    }

    void OnWorkerIteration() override {
        const int packetSize = _udp.parsePacket();

        if (packetSize > 0 && packetSize <= 256) {
            std::array<uint8_t, 256> packet{};

            const IPAddress source = _udp.remoteIP();
            const uint16_t sourcePort = _udp.remotePort();

            const int received = _udp.read(
                packet.data(),
                packet.size()
            );

            const uint64_t receiveTime =
                _protocol.GetLocalTimestamp();

            if (received > 0) {
                HandlePacket(
                    source,
                    sourcePort,
                    packet.data(),
                    static_cast<std::size_t>(received),
                    receiveTime
                );
            }
        }

        const uint64_t now = millis();

        if (
            _protocol.IsClientMode() &&
            _config.SynchronizationIntervalMilliseconds > 0 &&
            (
                _lastRequestMilliseconds == 0 ||
                now - _lastRequestMilliseconds >=
                    _config.SynchronizationIntervalMilliseconds
            )
        ) {
            RequestSynchronization();
        }

        if (
            _protocol.IsReferenceMode() &&
            (_config.EnableAuthoritativeBroadcast || _config.EnableAuthoritativeMulticast) &&
            _config.BroadcastIntervalMilliseconds > 0 &&
            (
                _lastBroadcastMilliseconds == 0 ||
                now - _lastBroadcastMilliseconds >=
                    _config.BroadcastIntervalMilliseconds
            )
        ) {
            BroadcastTime();
        }
    }

public:
    explicit UDPClockSynchronizer(
        Timing::IClockSynchronizationTarget<Timing::ClockTick>* target = nullptr
    ) :
        _protocol(target) {
    }

    ~UDPClockSynchronizer() {
        Shutdown();
    }

    bool Initialize(
        const UDPClockSynchronizationConfig& config
    ) {
        if (_initialized) {
            return true;
        }

        if (config.LocalPort == 0) {
            return false;
        }

        _config = config;
        _protocol.Configure(config);

        bool begun = false;

        if (config.EnableAuthoritativeMulticast) {
            begun = _udp.beginMulticast(
                config.MulticastGroup,
                config.MulticastPort
            );
        } else {
            begun = _udp.begin(config.LocalPort);
        }

        if (!begun) {
            return false;
        }

        SocketWorkerConfig worker;
        worker.StackSize = 4096;
        worker.Priority = 2;
        worker.IdleDelayMilliseconds = 1;

        if (!StartWorker("ESPressioUDPSync", worker)) {
            _udp.stop();
            return false;
        }

        _lastRequestMilliseconds = 0;
        _lastBroadcastMilliseconds = 0;
        _initialized = true;
        return true;
    }

    void Shutdown() {
        if (!_initialized) {
            return;
        }

        StopWorker();
        _udp.stop();
        _protocol.CancelPendingRequest();
        _initialized = false;
    }

    bool RequestSynchronization() {
        if (
            !_initialized ||
            !_protocol.IsClientMode() ||
            static_cast<uint32_t>(_config.ReferenceAddress) == 0 ||
            _config.ReferencePort == 0
        ) {
            return false;
        }

        SocketClockSynchronizationProtocol::RequestMessage request;

        if (!_protocol.BuildRequest(request)) {
            return false;
        }

        const bool sent = SendDatagram(
            _config.ReferenceAddress,
            _config.ReferencePort,
            reinterpret_cast<const uint8_t*>(&request),
            sizeof(request)
        );

        if (sent) {
            _lastRequestMilliseconds = millis();
        } else {
            _protocol.CancelPendingRequest();
        }

        return sent;
    }

    bool BroadcastTime() {
        if (!_initialized || !_protocol.IsReferenceMode()) {
            return false;
        }

        SocketClockSynchronizationProtocol::AuthoritativeBroadcastMessage message;

        if (!_protocol.BuildAuthoritativeBroadcast(message)) {
            return false;
        }

        bool sent = false;

        if (_config.EnableAuthoritativeBroadcast) {
            sent = SendDatagram(
                _config.BroadcastAddress,
                _config.LocalPort,
                reinterpret_cast<const uint8_t*>(&message),
                sizeof(message)
            ) || sent;
        }

        if (_config.EnableAuthoritativeMulticast) {
            sent = SendDatagram(
                _config.MulticastGroup,
                _config.MulticastPort,
                reinterpret_cast<const uint8_t*>(&message),
                sizeof(message)
            ) || sent;
        }

        if (sent) {
            _lastBroadcastMilliseconds = millis();
        }

        return sent;
    }

    bool GetIsInitialized() const noexcept {
        return _initialized;
    }

    Timing::ClockSynchronizationStatus<Timing::ClockTick>
    GetSynchronizationStatus() const {
        return _protocol.GetSynchronizationStatus();
    }
};

} // namespace ESPressio::Sockets
