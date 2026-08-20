#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include <ESPressio_Security.hpp>

namespace ESPressio::Sockets {

class SocketSecurityDatagram final {
public:
    using SendCallback = std::function<bool(const uint8_t*, std::size_t)>;
    using ReceiveCallback = std::function<void(const Security::UnprotectedPayload&)>;
    using FailureCallback = std::function<void(const Security::SecurityResult&)>;

    SocketSecurityDatagram(Security::TransportSecurity& security, SendCallback sender)
        : _security(security), _sender(std::move(sender)) {}

    void SetReceiveCallback(ReceiveCallback callback) { _receive = std::move(callback); }
    void SetFailureCallback(FailureCallback callback) { _failure = std::move(callback); }

    bool Send(uint8_t protocol, const void* payload, std::size_t payloadLength, Security::SecurityResult* resultOut = nullptr) {
        if (!_sender || (payload == nullptr && payloadLength != 0)) return false;
        std::vector<uint8_t> protectedBytes;
        auto result = _security.Protect(protocol, static_cast<const uint8_t*>(payload), payloadLength, protectedBytes);
        if (resultOut) *resultOut = result;
        return result.Success && _sender(protectedBytes.data(), protectedBytes.size());
    }

    bool Receive(const uint8_t* datagram, std::size_t size) {
        if (datagram == nullptr || size == 0) return false;
        Security::UnprotectedPayload opened;
        const uint8_t protocol = size > 7 ? datagram[7] : 0;
        auto result = _security.Unprotect(protocol, datagram, size, opened);
        if (!result.Success) {
            if (_failure) _failure(result);
            return false;
        }
        if (_receive) _receive(opened);
        return true;
    }

private:
    Security::TransportSecurity& _security;
    SendCallback _sender;
    ReceiveCallback _receive;
    FailureCallback _failure;
};

}
