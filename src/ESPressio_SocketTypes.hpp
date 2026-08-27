#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <ESPressio_Platform.hpp>

namespace ESPressio::Sockets {

struct IPv4Address {
    std::array<uint8_t, 4> Octets{};

    constexpr IPv4Address() = default;
    constexpr IPv4Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        : Octets{{a, b, c, d}} {}

    constexpr bool IsZero() const noexcept {
        return Octets[0] == 0 && Octets[1] == 0 &&
               Octets[2] == 0 && Octets[3] == 0;
    }

    constexpr bool operator==(const IPv4Address& other) const noexcept {
        return Octets == other.Octets;
    }

    constexpr bool operator!=(const IPv4Address& other) const noexcept {
        return !(*this == other);
    }
};

struct SocketEndpoint {
    IPv4Address Address;
    uint16_t Port = 0;

    SocketEndpoint() = default;
    constexpr SocketEndpoint(const IPv4Address& address, uint16_t port)
        : Address(address), Port(port) {}

    constexpr bool IsValid() const noexcept {
        return Port != 0 && !Address.IsZero();
    }

    constexpr bool operator==(const SocketEndpoint& other) const noexcept {
        return Address == other.Address && Port == other.Port;
    }

    constexpr bool operator!=(const SocketEndpoint& other) const noexcept {
        return !(*this == other);
    }
};

struct SocketWorkerConfig {
    uint32_t StackSize = 4096;
    uint32_t Priority = 2;
    System::ProcessorAffinity Affinity = System::ProcessorAffinity::Any();
    uint32_t IdleDelayMilliseconds = 2;
};

#ifndef ESPRESSIO_SOCKETS_MAX_EVENT_PACKET_SIZE
    #define ESPRESSIO_SOCKETS_MAX_EVENT_PACKET_SIZE 65536
#endif

#ifndef ESPRESSIO_SOCKETS_MAX_UDP_DESTINATIONS
    #define ESPRESSIO_SOCKETS_MAX_UDP_DESTINATIONS 16
#endif

#ifndef ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS
    #define ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS 8
#endif

} // namespace ESPressio::Sockets
