#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>

#include <ESPressio_IClockSynchronizationTarget.hpp>
#include <ESPressio_SystemClock.hpp>

#include "ESPressio_SocketClockSynchronizationTypes.hpp"

namespace ESPressio::Sockets {

class SocketClockSynchronizationProtocol {
public:
    enum class MessageType : uint8_t {
        Request = 1,
        Response = 2,
        AuthoritativeBroadcast = 3
    };

#pragma pack(push, 1)
    struct MessageHeader {
        static constexpr uint32_t MagicValue = 0x53434C4Bu; // SCLK
        uint32_t Magic = MagicValue;
        uint8_t Version = 1;
        uint8_t Type = 0;
        uint16_t Reserved = 0;
        uint32_t Sequence = 0;
    };

    struct RequestMessage {
        MessageHeader Header;
        uint64_t T1 = 0;
    };

    struct ResponseMessage {
        MessageHeader Header;
        uint64_t T1 = 0;
        uint64_t T2 = 0;
        uint64_t T3 = 0;
    };

    struct AuthoritativeBroadcastMessage {
        MessageHeader Header;
        uint64_t ReferenceTransmitTime = 0;
    };
#pragma pack(pop)

private:
    Timing::IClockSynchronizationTarget<Timing::ClockTick>* _target = nullptr;
    SocketClockSynchronizationConfig _config;
    std::atomic<uint32_t> _nextSequence{1};
    std::atomic<uint32_t> _pendingSequence{0};

public:
    explicit SocketClockSynchronizationProtocol(
        Timing::IClockSynchronizationTarget<Timing::ClockTick>* target = nullptr
    ) :
        _target(
            target == nullptr
                ? static_cast<Timing::IClockSynchronizationTarget<Timing::ClockTick>*>(
                    &Timing::SystemClock<>::GetInstance()
                  )
                : target
        ) {
    }

    void Configure(const SocketClockSynchronizationConfig& config) {
        _config = config;
    }

    const SocketClockSynchronizationConfig& GetConfig() const noexcept {
        return _config;
    }

    uint64_t GetLocalTimestamp() const {
        return _target == nullptr
            ? 0
            : _target->GetSynchronizationTimestampNanoseconds();
    }

    bool IsClientMode() const noexcept {
        return _config.Mode == SocketClockSynchronizationMode::Client ||
               _config.Mode == SocketClockSynchronizationMode::ClientAndReference;
    }

    bool IsReferenceMode() const noexcept {
        return _config.Mode == SocketClockSynchronizationMode::Reference ||
               _config.Mode == SocketClockSynchronizationMode::ClientAndReference;
    }

    uint32_t GetPendingSequence() const noexcept {
        return _pendingSequence.load(std::memory_order_acquire);
    }

    void CancelPendingRequest() noexcept {
        _pendingSequence.store(0, std::memory_order_release);
    }

    bool BuildRequest(RequestMessage& request) {
        if (!IsClientMode() || _target == nullptr) {
            return false;
        }

        request.Header.Type = static_cast<uint8_t>(MessageType::Request);
        request.Header.Sequence = _nextSequence.fetch_add(1);
        request.T1 = _target->GetSynchronizationTimestampNanoseconds();
        _pendingSequence.store(request.Header.Sequence, std::memory_order_release);
        return true;
    }

    bool ProcessRequest(
        const uint8_t* data,
        std::size_t size,
        uint64_t localReceiveTime,
        const std::function<bool(const uint8_t*, std::size_t)>& sendResponse
    ) {
        if (!IsReferenceMode() || data == nullptr || size != sizeof(RequestMessage)) {
            return false;
        }

        RequestMessage request;
        std::memcpy(&request, data, sizeof(request));

        if (request.Header.Magic != MessageHeader::MagicValue ||
            request.Header.Version != 1 ||
            request.Header.Type != static_cast<uint8_t>(MessageType::Request)) {
            return false;
        }

        ResponseMessage response;
        response.Header.Type = static_cast<uint8_t>(MessageType::Response);
        response.Header.Sequence = request.Header.Sequence;
        response.T1 = request.T1;
        response.T2 = localReceiveTime;

        // T3 deliberately captured immediately before transport handoff.
        response.T3 = _target->GetSynchronizationTimestampNanoseconds();

        return sendResponse(
            reinterpret_cast<const uint8_t*>(&response),
            sizeof(response)
        );
    }

    bool ProcessResponse(
        const uint8_t* data,
        std::size_t size,
        uint64_t localReceiveTime
    ) {
        if (!IsClientMode() || data == nullptr || size != sizeof(ResponseMessage)) {
            return false;
        }

        ResponseMessage response;
        std::memcpy(&response, data, sizeof(response));

        if (response.Header.Magic != MessageHeader::MagicValue ||
            response.Header.Version != 1 ||
            response.Header.Type != static_cast<uint8_t>(MessageType::Response)) {
            return false;
        }

        const uint32_t pending = _pendingSequence.load(std::memory_order_acquire);
        if (pending == 0 || response.Header.Sequence != pending) {
            return false;
        }

        _pendingSequence.store(0, std::memory_order_release);

        Timing::ClockSynchronizationSample<Timing::ClockTick> sample;
        sample.LocalRequestTransmitTime = response.T1;
        sample.RemoteRequestReceiveTime = response.T2;
        sample.RemoteResponseTransmitTime = response.T3;
        sample.LocalResponseReceiveTime = localReceiveTime;

        return _target->SubmitSynchronizationSample(
            sample,
            _config.AdjustmentMode
        ).Accepted;
    }

    bool BuildAuthoritativeBroadcast(AuthoritativeBroadcastMessage& message) {
        if (!IsReferenceMode() || _target == nullptr) {
            return false;
        }

        message.Header.Type = static_cast<uint8_t>(MessageType::AuthoritativeBroadcast);
        message.Header.Sequence = _nextSequence.fetch_add(1);
        message.ReferenceTransmitTime = _target->GetSynchronizationTimestampNanoseconds();
        return true;
    }

    bool ProcessAuthoritativeBroadcast(
        const uint8_t* data,
        std::size_t size,
        uint64_t localReceiveTime
    ) {
        if (!IsClientMode() || data == nullptr || size != sizeof(AuthoritativeBroadcastMessage)) {
            return false;
        }

        AuthoritativeBroadcastMessage message;
        std::memcpy(&message, data, sizeof(message));

        if (message.Header.Magic != MessageHeader::MagicValue ||
            message.Header.Version != 1 ||
            message.Header.Type != static_cast<uint8_t>(MessageType::AuthoritativeBroadcast)) {
            return false;
        }

        /*
         * One-way authoritative synchronization cannot compensate for network
         * latency. Represent the observation as a zero-duration local/remote
         * exchange so Timing applies the measured phase offset while retaining
         * all normal discipline/Observer behavior.
         */
        Timing::ClockSynchronizationSample<Timing::ClockTick> sample;
        sample.LocalRequestTransmitTime = localReceiveTime;
        sample.LocalResponseReceiveTime = localReceiveTime;
        sample.RemoteRequestReceiveTime = message.ReferenceTransmitTime;
        sample.RemoteResponseTransmitTime = message.ReferenceTransmitTime;

        return _target->SubmitSynchronizationSample(
            sample,
            _config.AdjustmentMode
        ).Accepted;
    }

    Timing::ClockSynchronizationStatus<Timing::ClockTick>
    GetSynchronizationStatus() const {
        return _target->GetSynchronizationStatus();
    }
};

}
