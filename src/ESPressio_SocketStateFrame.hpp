#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace ESPressio::Sockets {

#pragma pack(push, 1)
struct SocketStateFrameHeader final {
    static constexpr uint32_t MagicValue = 0x4B535453u; // "STSK" on the wire
    static constexpr uint8_t VersionValue = 1;

    uint32_t Magic = MagicValue;
    uint8_t Version = VersionValue;
    uint8_t Reserved[3] = {0, 0, 0};
    uint32_t PayloadLength = 0;
};
#pragma pack(pop)

static_assert(
    sizeof(SocketStateFrameHeader) == 12,
    "SocketStateFrameHeader wire layout changed."
);

class SocketStateFrameDecoder final {
public:
    explicit SocketStateFrameDecoder(std::size_t maximumPayloadBytes = 4096)
        : _maximumPayloadBytes(maximumPayloadBytes) {}

    void SetMaximumPayloadBytes(std::size_t value) {
        _maximumPayloadBytes = value;
        if (_buffer.size() > _maximumPayloadBytes + sizeof(SocketStateFrameHeader)) {
            _buffer.clear();
        }
    }

    std::size_t MaximumPayloadBytes() const noexcept {
        return _maximumPayloadBytes;
    }

    template<typename TCallback>
    bool Push(const uint8_t* data, std::size_t size, TCallback&& callback) {
        if (data == nullptr || size == 0) return true;
        if (_maximumPayloadBytes == 0 ||
            _buffer.size() + size > _maximumPayloadBytes + sizeof(SocketStateFrameHeader)) {
            _buffer.clear();
            return false;
        }

        _buffer.insert(_buffer.end(), data, data + size);

        for (;;) {
            if (_buffer.size() < sizeof(SocketStateFrameHeader)) return true;

            SocketStateFrameHeader header{};
            std::memcpy(&header, _buffer.data(), sizeof(header));
            if (header.Magic != SocketStateFrameHeader::MagicValue ||
                header.Version != SocketStateFrameHeader::VersionValue ||
                header.PayloadLength == 0 ||
                header.PayloadLength > _maximumPayloadBytes) {
                _buffer.clear();
                return false;
            }

            const std::size_t payloadSize = static_cast<std::size_t>(header.PayloadLength);
            const std::size_t frameSize = sizeof(SocketStateFrameHeader) + payloadSize;
            if (_buffer.size() < frameSize) return true;

            // Consume the frame before invoking application/session code. A callback may
            // synchronously feed another frame into this decoder; leaving the current
            // frame at the front of _buffer would cause that nested Push() to decode the
            // same frame again. Keep the callback payload in independent storage so the
            // nested Push() may freely mutate/reallocate _buffer without invalidating it.
            std::vector<uint8_t> payload(payloadSize);
            std::memcpy(
                payload.data(),
                _buffer.data() + sizeof(SocketStateFrameHeader),
                payloadSize
            );
            _buffer.erase(
                _buffer.begin(),
                _buffer.begin() + static_cast<std::ptrdiff_t>(frameSize)
            );

            callback(payload.data(), payload.size());
        }
    }

    void Reset() { _buffer.clear(); }

private:
    std::size_t _maximumPayloadBytes = 4096;
    std::vector<uint8_t> _buffer;
};

inline std::vector<uint8_t> BuildSocketStateFrame(
    const uint8_t* payload,
    std::size_t payloadSize,
    std::size_t maximumPayloadBytes = 4096
) {
    if (payload == nullptr || payloadSize == 0 ||
        maximumPayloadBytes == 0 || payloadSize > maximumPayloadBytes ||
        payloadSize > 0xFFFFFFFFu) {
        return {};
    }

    SocketStateFrameHeader header{};
    header.PayloadLength = static_cast<uint32_t>(payloadSize);

    std::vector<uint8_t> frame(sizeof(header) + payloadSize);
    std::memcpy(frame.data(), &header, sizeof(header));
    std::memcpy(frame.data() + sizeof(header), payload, payloadSize);
    return frame;
}

} // namespace ESPressio::Sockets
