#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <ESPressio_ThreadSafeObservable.hpp>
#include <ESPressio_Security.hpp>

#include "ESPressio_ISocketSecuritySessionObserver.hpp"

namespace ESPressio::Sockets {

struct SocketSecuritySessionConfig {
    std::size_t MaximumProtectedFrameBytes = 65536;
};

class SocketSecuritySession final {
public:
    using WriteCallback = std::function<bool(const uint8_t*, std::size_t)>;
    using ReceiveCallback = std::function<void(const Security::UnprotectedPayload&)>;
    using FailureCallback = std::function<void(const Security::SecurityResult&)>;

private:
    class SessionObservable final : public Observable::ThreadSafeObservable {
    private:
        template <typename Callback>
        void Notify(Callback&& callback) {
            ExecuteNotification([&](NotificationContext& notification) {
                notification.WithObservers<ISocketSecuritySessionObserver>([&](ISocketSecuritySessionObserver* observer) {
                    try { callback(observer); } catch (...) {}
                });
            });
        }
    public:
        void Faulted(const Security::SecurityResult& result) {
            Notify([&](ISocketSecuritySessionObserver* observer){ observer->OnSocketSecuritySessionFaulted(result); });
        }
        void Reset() {
            Notify([](ISocketSecuritySessionObserver* observer){ observer->OnSocketSecuritySessionReset(); });
        }
    };

    Security::TransportSecurity& _security;
    WriteCallback _writer;
    SocketSecuritySessionConfig _config;
    ReceiveCallback _receive;
    FailureCallback _failure;
    std::vector<uint8_t> _buffer;
    bool _discarding = false;
    std::shared_ptr<SessionObservable> _observable = std::make_shared<SessionObservable>();

    void PublishFailure(const Security::SecurityResult& failure) {
        if (_failure) _failure(failure);
        _observable->Faulted(failure);
    }

    void ProcessEnvelope(uint8_t protocol, const uint8_t* envelope, std::size_t size) {
        Security::UnprotectedPayload opened;
        auto result = _security.Unprotect(protocol, envelope, size, opened);
        if (!result.Success) {
            PublishFailure(result);
            return;
        }
        if (_receive) _receive(opened);
    }

    static void Append32(std::vector<uint8_t>& out, uint32_t value) {
        for (int i=0;i<4;++i) out.push_back(static_cast<uint8_t>(value >> (i*8)));
    }
    static uint32_t Read32(const uint8_t* p) {
        return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1])<<8) |
               (static_cast<uint32_t>(p[2])<<16) | (static_cast<uint32_t>(p[3])<<24);
    }

public:
    SocketSecuritySession(Security::TransportSecurity& security, WriteCallback writer, SocketSecuritySessionConfig config = {})
        : _security(security), _writer(std::move(writer)), _config(config) {}

    void SetReceiveCallback(ReceiveCallback callback) { _receive = std::move(callback); }
    void SetFailureCallback(FailureCallback callback) { _failure = std::move(callback); }

    Observable::ObserverHandlePtr RegisterObserver(ISocketSecuritySessionObserver* observer) {
        return _observable->RegisterObserver(observer);
    }

    void UnregisterObserver(ISocketSecuritySessionObserver* observer) {
        _observable->UnregisterObserver(observer);
    }

    bool Send(uint8_t protocol, const void* payload, std::size_t payloadLength, Security::SecurityResult* resultOut = nullptr) {
        if (!_writer || (payload == nullptr && payloadLength != 0)) return false;
        std::vector<uint8_t> protectedBytes;
        auto result = _security.Protect(protocol, static_cast<const uint8_t*>(payload), payloadLength, protectedBytes);
        if (resultOut) *resultOut = result;
        if (!result.Success) {
            PublishFailure(result);
            return false;
        }
        if (protectedBytes.empty() || protectedBytes.size() > _config.MaximumProtectedFrameBytes || protectedBytes.size() > 0xFFFFFFFFu) {
            auto failure = Security::SecurityResult::Fail(Security::SecurityError::BufferLimitExceeded, "Protected secure socket frame is empty or exceeds configured limit");
            if (resultOut) *resultOut = failure;
            PublishFailure(failure);
            return false;
        }
        std::vector<uint8_t> frame;
        frame.reserve(5 + protectedBytes.size());
        Append32(frame, static_cast<uint32_t>(protectedBytes.size()));
        frame.push_back(protocol);
        frame.insert(frame.end(), protectedBytes.begin(), protectedBytes.end());
        return _writer(frame.data(), frame.size());
    }

    bool Feed(const uint8_t* data, std::size_t size) {
        if ((data == nullptr && size != 0) || _discarding) return false;
        if (size) _buffer.insert(_buffer.end(), data, data + size);
        while (true) {
            if (_buffer.size() < 5) return true;
            const uint32_t length = Read32(_buffer.data());
            if (length == 0 || length > _config.MaximumProtectedFrameBytes) {
                _buffer.clear();
                _discarding = true;
                auto failure = Security::SecurityResult::Fail(Security::SecurityError::BufferLimitExceeded, "Secure socket frame length is invalid or exceeds configured limit");
                PublishFailure(failure);
                return false;
            }
            if (_buffer.size() < 5 + static_cast<std::size_t>(length)) return true;
            const uint8_t protocol = _buffer[4];
            ProcessEnvelope(protocol, _buffer.data() + 5, length);
            _buffer.erase(_buffer.begin(), _buffer.begin() + 5 + length);
        }
    }

    void Reset() {
        _buffer.clear();
        _discarding = false;
        _observable->Reset();
    }

    std::size_t BufferedBytes() const noexcept { return _buffer.size(); }
};

} // namespace ESPressio::Sockets
