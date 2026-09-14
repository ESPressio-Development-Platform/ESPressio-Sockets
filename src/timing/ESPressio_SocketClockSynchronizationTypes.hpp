#pragma once

#include <cstddef>
#include <cstdint>
#include <ESPressio_ClockSynchronization.hpp>

namespace ESPressio::Sockets {

enum class SocketClockSynchronizationMode : std::uint8_t {
    Client = 0,
    Reference = 1,
    ClientAndReference = 2
};

/// <summary>Finite transport-composition facts for one direct socket clock exchange.</summary>
/// <remarks>Timing owns estimator, discipline, reliability and adaptive scheduling. No fixed synchronization cadence lives here.</remarks>
struct SocketClockSynchronizationConfig final {
    SocketClockSynchronizationMode Mode{SocketClockSynchronizationMode::Client};
    std::uint64_t ReferenceIdentity{0};
    std::uint64_t RequestTimeoutNanoseconds{1'500'000'000ULL};
    Timing::ClockCaptureQuality LocalTransmitCaptureQuality{Timing::ClockCaptureQuality::SoftwareUnbounded};
    Timing::ClockUncertainty LocalTransmitCaptureUncertainty{};
};

struct SocketClockResponseSink final {
    void* Owner{nullptr};
    bool (*Send)(void*,const std::uint8_t*,std::size_t) noexcept{nullptr};
    constexpr explicit operator bool() const noexcept { return Owner!=nullptr && Send!=nullptr; }
};

} // namespace ESPressio::Sockets
