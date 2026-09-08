#pragma once

#include <cstdint>

#include <ESPressio_ClockSynchronization.hpp>

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
enum
class SocketClockSynchronizationMode : uint8_t {
    Client,
    Reference,
    ClientAndReference
};

/**
 * ESPressio Memory Audit
 * Members:
 * - Mode (SocketClockSynchronizationMode): 1 bytes [0 bytes dynamic allocation]
 * - SynchronizationIntervalMilliseconds (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - RequestTimeoutMilliseconds (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - AdjustmentMode (Timing::ClockSynchronizationAdjustmentMode): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 16 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
struct SocketClockSynchronizationConfig {
    SocketClockSynchronizationMode Mode =
        SocketClockSynchronizationMode::Client;

    uint32_t SynchronizationIntervalMilliseconds = 5000;
    uint32_t RequestTimeoutMilliseconds = 1500;

    Timing::ClockSynchronizationAdjustmentMode AdjustmentMode =
        Timing::ClockSynchronizationAdjustmentMode::StepIfUnsynchronized;
};

} // namespace ESPressio::Sockets
