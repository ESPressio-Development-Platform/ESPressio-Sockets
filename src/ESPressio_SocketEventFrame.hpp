#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "ESPressio_SocketTypes.hpp"

namespace ESPressio::Sockets {

#pragma pack(push, 1)
struct SocketEventFrameHeader {
    static constexpr uint32_t MagicValue =
        0x4556534Bu; // EVSK

    static constexpr uint8_t VersionValue = 1;

    uint32_t Magic = MagicValue;
    uint8_t Version = VersionValue;
    uint8_t Reserved[3] = {0, 0, 0};
    uint32_t PayloadLength = 0;
};
#pragma pack(pop)

static_assert(
    sizeof(SocketEventFrameHeader) == 12,
    "SocketEventFrameHeader wire layout changed."
);


class SocketEventFrameDecoder {
private:
    std::vector<uint8_t> _buffer;

public:
    template<typename TCallback>
    bool Push(
        const uint8_t* data,
        std::size_t size,
        TCallback callback
    ) {
        if (
            data == nullptr ||
            size == 0
        ) {
            return true;
        }

        if (
            _buffer.size() + size >
            ESPRESSIO_SOCKETS_MAX_EVENT_PACKET_SIZE +
                sizeof(SocketEventFrameHeader)
        ) {
            _buffer.clear();
            return false;
        }

        _buffer.insert(
            _buffer.end(),
            data,
            data + size
        );

        for (;;) {
            if (
                _buffer.size() <
                sizeof(SocketEventFrameHeader)
            ) {
                return true;
            }

            SocketEventFrameHeader header;

            std::memcpy(
                &header,
                _buffer.data(),
                sizeof(header)
            );

            if (
                header.Magic !=
                    SocketEventFrameHeader::MagicValue ||
                header.Version !=
                    SocketEventFrameHeader::VersionValue ||
                header.PayloadLength == 0 ||
                header.PayloadLength >
                    ESPRESSIO_SOCKETS_MAX_EVENT_PACKET_SIZE
            ) {
                _buffer.clear();
                return false;
            }

            const std::size_t frameSize =
                sizeof(SocketEventFrameHeader) +
                header.PayloadLength;

            if (
                _buffer.size() <
                frameSize
            ) {
                return true;
            }

            callback(
                _buffer.data() +
                    sizeof(SocketEventFrameHeader),
                header.PayloadLength
            );

            _buffer.erase(
                _buffer.begin(),
                _buffer.begin() +
                    static_cast<std::ptrdiff_t>(frameSize)
            );
        }
    }

    void Reset() {
        _buffer.clear();
    }
};


inline std::vector<uint8_t>
BuildSocketEventFrame(
    const uint8_t* data,
    std::size_t size
) {
    if (
        data == nullptr ||
        size == 0 ||
        size >
            ESPRESSIO_SOCKETS_MAX_EVENT_PACKET_SIZE
    ) {
        return {};
    }

    SocketEventFrameHeader header;

    header.PayloadLength =
        static_cast<uint32_t>(size);

    std::vector<uint8_t> frame(
        sizeof(header) + size
    );

    std::memcpy(
        frame.data(),
        &header,
        sizeof(header)
    );

    std::memcpy(
        frame.data() + sizeof(header),
        data,
        size
    );

    return frame;
}

}
