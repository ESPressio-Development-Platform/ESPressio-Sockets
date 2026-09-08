#pragma once

#include <cstdint>
#include <IPAddress.h>

#include "ESPressio_SocketClockSynchronizationTypes.hpp"

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 16 bytes [0 bytes dynamic allocation]
 * Members:
 * - LocalPort (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * - ReferencePort (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * - ReferenceAddress (IPAddress): 4 bytes [0 bytes dynamic allocation]
 * - EnableAuthoritativeBroadcast (bool): 1 bytes [0 bytes dynamic allocation]
 * - BroadcastIntervalMilliseconds (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - EnableAuthoritativeMulticast (bool): 1 bytes [0 bytes dynamic allocation]
 * - MulticastPort (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * Total Memory: 36 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
struct UDPClockSynchronizationConfig :
    public SocketClockSynchronizationConfig {

    uint16_t LocalPort = 45100;
    uint16_t ReferencePort = 45100;

    IPAddress ReferenceAddress;

    bool EnableAuthoritativeBroadcast = false;
    IPAddress BroadcastAddress = IPAddress(255, 255, 255, 255);
    uint32_t BroadcastIntervalMilliseconds = 5000;

    bool EnableAuthoritativeMulticast = false;
    IPAddress MulticastGroup = IPAddress(239, 45, 10, 1);
    uint16_t MulticastPort = 45100;
};

} // namespace ESPressio::Sockets
