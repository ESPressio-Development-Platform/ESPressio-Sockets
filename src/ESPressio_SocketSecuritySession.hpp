#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include <ESPressio_Security.hpp>

namespace ESPressio::Sockets {

struct SocketSecuritySessionConfig {
    std::size_t MaximumProtectedFrameBytes = 65536;
};

class SocketSecuritySession final {
public:
    using WriteCallback = std::function<bool(const uint8_t*, std::size_t)>;
    using ReceiveCallback = std::function<void(const Security::UnprotectedPayload&)>;
    using FailureCallback = std::function<void(const Security::SecurityResult&)>;

    SocketSecuritySession(Security::TransportSecurity& security, WriteCallback writer, SocketSecuritySessionConfig config = {})
        : _security(security), _writer(std::move(writer)), _config(config) {}

    void SetReceiveCallback(ReceiveCallback callback) { _receive = std::move(callback); }
    void SetFailureCallback(FailureCallback callback) { _failure = std::move(callback); }

    bool Send(uint8_t protocol, const void* payload, std::size_t payloadLength, Security::SecurityResult* resultOut = nullptr) {
        if (!_writer || (payload == nullptr && payloadLength != 0)) return false;
        std::vector<uint8_t> protectedBytes;
        auto result = _security.Protect(protocol, static_cast<const uint8_t*>(payload), payloadLength, protectedBytes);
        if (resultOut) *resultOut = result;
        if (!result.Success || protectedBytes.empty() || protectedBytes.size() > _config.MaximumProtectedFrameBytes || protectedBytes.size() > 0xFFFFFFFFu) return false;
        std::vector<uint8_t> frame;
        frame.reserve(4 + protectedBytes.size());
        Append32(frame, static_cast<uint32_t>(protectedBytes.size()));
        frame.insert(frame.end(), protectedBytes.begin(), protectedBytes.end());
        return _writer(frame.data(), frame.size());
    }

    bool Feed(const uint8_t* data, std::size_t size) {
        if ((data == nullptr && size != 0) || _discarding) return false;
        if (size) _buffer.insert(_buffer.end(), data, data + size);
        while (true) {
            if (_buffer.size() < 4) return true;
            const uint32_t length = Read32(_buffer.data());
            if (length == 0 || length > _config.MaximumProtectedFrameBytes) {
                _buffer.clear(); _discarding = true;
                auto failure = Security::SecurityResult::Fail(Security::SecurityError::BufferLimitExceeded, "Secure socket frame length is invalid or exceeds configured limit");
                if (_failure) _failure(failure);
                return false;
            }
            if (_buffer.size() < 4 + static_cast<std::size_t>(length)) return true;
            ProcessEnvelope(_buffer.data() + 4, length);
            _buffer.erase(_buffer.begin(), _buffer.begin() + 4 + length);
        }
    }

    void Reset() { _buffer.clear(); _discarding = false; }
    std::size_t BufferedBytes() const noexcept { return _buffer.size(); }

private:
    Security::TransportSecurity& _security;
    WriteCallback _writer;
    SocketSecuritySessionConfig _config;
    ReceiveCallback _receive;
    FailureCallback _failure;
    std::vector<uint8_t> _buffer;
    bool _discarding = false;

    static uint8_t CandidateProtocol(const uint8_t* envelope, std::size_t size) { return size > 7 ? envelope[7] : 0; }

    void ProcessEnvelope(const uint8_t* envelope, std::size_t size) {
        Security::UnprotectedPayload opened;
        const uint8_t protocol = CandidateProtocol(envelope, size);
        auto result = _security.Unprotect(protocol, envelope, size, opened);
        if (!result.Success) { if (_failure) _failure(result); return; }
        if (_receive) _receive(opened);
    }

    static void Append32(std::vector<uint8_t>& out, uint32_t value) {
        for (int i=0;i<4;++i) out.push_back(static_cast<uint8_t>(value >> (i*8)));
    }
    static uint32_t Read32(const uint8_t* p) {
        return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1])<<8) |
               (static_cast<uint32_t>(p[2])<<16) | (static_cast<uint32_t>(p[3])<<24);
    }
};

}
