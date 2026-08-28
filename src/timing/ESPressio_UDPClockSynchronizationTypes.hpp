#pragma once

#include <cstdint>
#include <IPAddress.h>

#include "ESPressio_SocketClockSynchronizationTypes.hpp"

namespace ESPressio::Sockets {

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
