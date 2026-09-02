#pragma once

#include <cstddef>
#include <cstdint>

#include <Arduino.h>
#include <IPAddress.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace ESPressio::Sockets {

struct SocketEndpoint {
    IPAddress Address;
    uint16_t Port = 0;

    SocketEndpoint() = default;

    SocketEndpoint(
        const IPAddress& address,
        uint16_t port
    ) :
        Address(address),
        Port(port) {
    }

    bool IsValid() const noexcept {
        return
            Port != 0 &&
            static_cast<uint32_t>(Address) != 0;
    }

    bool operator==(
        const SocketEndpoint& other
    ) const noexcept {
        return
            Address == other.Address &&
            Port == other.Port;
    }

    bool operator!=(
        const SocketEndpoint& other
    ) const noexcept {
        return !(*this == other);
    }
};


struct SocketWorkerConfig {
    uint32_t StackSize = 4096;
    UBaseType_t Priority = 2;
    BaseType_t Core = tskNO_AFFINITY;
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

}
