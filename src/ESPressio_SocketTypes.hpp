#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <ESPressio_Platform.hpp>

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Members:
 * - Octets (std::array<uint8_t, 4>): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
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
        return Octets[0] == other.Octets[0] &&
               Octets[1] == other.Octets[1] &&
               Octets[2] == other.Octets[2] &&
               Octets[3] == other.Octets[3];
    }

    constexpr bool operator!=(const IPv4Address& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * ESPressio Memory Audit
 * Members:
 * - Address (IPv4Address): 4 bytes [0 bytes dynamic allocation]
 * - Port (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * Total Memory: 6 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
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

/**
 * ESPressio Memory Audit
 * Members:
 * - StackSize (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - Priority (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - Affinity (System::ProcessorAffinity): 2 bytes [0 bytes dynamic allocation]
 * - IdleDelayMilliseconds (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 16 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
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
