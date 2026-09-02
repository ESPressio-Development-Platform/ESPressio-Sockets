#pragma once

#include <cstdint>

#include <ESPressio_ClockSynchronization.hpp>

namespace ESPressio::Sockets {

enum class SocketClockSynchronizationMode : uint8_t {
    Client,
    Reference,
    ClientAndReference
};

struct SocketClockSynchronizationConfig {
    SocketClockSynchronizationMode Mode =
        SocketClockSynchronizationMode::Client;

    uint32_t SynchronizationIntervalMilliseconds = 5000;
    uint32_t RequestTimeoutMilliseconds = 1500;

    Timing::ClockSynchronizationAdjustmentMode AdjustmentMode =
        Timing::ClockSynchronizationAdjustmentMode::StepIfUnsynchronized;
};

} // namespace ESPressio::Sockets
