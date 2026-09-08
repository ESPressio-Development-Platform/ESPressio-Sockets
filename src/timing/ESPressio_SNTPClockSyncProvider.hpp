#pragma once

#include <cstdint>
#include <mutex>

#include <Arduino.h>
#include <esp_sntp.h>

#include <ESPressio_IClockSynchronizationTarget.hpp>
#include <ESPressio_SystemClock.hpp>

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Members:
 * - Server (String): 12 bytes [Capacity + 1 bytes backing buffer when allocated]
 * - UpdateIntervalMilliseconds (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * - AdjustmentMode (Timing::ClockSynchronizationAdjustmentMode): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 20 bytes [Server: Capacity + 1 bytes backing buffer when allocated]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct SNTPClockSyncProviderConfig {
    String Server = "pool.ntp.org";
    uint32_t UpdateIntervalMilliseconds = 3600000;

    Timing::ClockSynchronizationAdjustmentMode AdjustmentMode =
        Timing::ClockSynchronizationAdjustmentMode::StepIfUnsynchronized;
};


/**
 * ESPressio Memory Audit
 * Members:
 * - _target (Timing::IClockSynchronizationTarget<Timing::ClockTick>*): 4 bytes [0 bytes dynamic allocation]
 * - _config (SNTPClockSyncProviderConfig): 20 bytes [Server: Capacity + 1 bytes backing buffer when allocated]
 * - _initialized (bool): 1 bytes [0 bytes dynamic allocation]
 * - _mutex (std::mutex): 4 bytes [native synchronization state may allocate platform resources lazily]
 * Total Memory: 32 bytes [_config: Server: Capacity + 1 bytes backing buffer when allocated; _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SNTPClockSyncProvider final {
private:
    Timing::IClockSynchronizationTarget<Timing::ClockTick>* _target = nullptr;
    SNTPClockSyncProviderConfig _config;
    bool _initialized = false;
    mutable std::mutex _mutex;

    inline static SNTPClockSyncProvider* _activeInstance = nullptr;

    static void TimeSyncCallback(struct timeval* tv) {
        SNTPClockSyncProvider* instance = nullptr;

        {
            instance = _activeInstance;
        }

        if (instance != nullptr && tv != nullptr) {
            instance->HandleSynchronizedTime(*tv);
        }
    }

    void HandleSynchronizedTime(const struct timeval& tv) {
        Timing::IClockSynchronizationTarget<Timing::ClockTick>* target = nullptr;
        Timing::ClockSynchronizationAdjustmentMode adjustmentMode;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_initialized || _target == nullptr) {
                return;
            }

            target = _target;
            adjustmentMode = _config.AdjustmentMode;
        }

        const uint64_t localTime =
            target->GetSynchronizationTimestampNanoseconds();

        const uint64_t remoteUnixNanoseconds =
            static_cast<uint64_t>(tv.tv_sec) * 1000000000ULL +
            static_cast<uint64_t>(tv.tv_usec) * 1000ULL;

        /*
         * The ESP-IDF SNTP callback supplies the synchronized reference time,
         * but not the four packet timestamps used internally by NTP. Feed the
         * reference into Timing as a zero-duration observation. Timing still
         * owns stepping/slewing, state, filtering and Observer notifications.
         *
         * This establishes ESPressio System Clock in the Unix epoch domain.
         */
        Timing::ClockSynchronizationSample<Timing::ClockTick> sample;
        sample.LocalRequestTransmitTime = localTime;
        sample.LocalResponseReceiveTime = localTime;
        sample.RemoteRequestReceiveTime = remoteUnixNanoseconds;
        sample.RemoteResponseTransmitTime = remoteUnixNanoseconds;

        target->SubmitSynchronizationSample(
            sample,
            adjustmentMode
        );
    }

public:
    explicit SNTPClockSyncProvider(
        Timing::IClockSynchronizationTarget<Timing::ClockTick>* target = nullptr
    ) :
        _target(
            target == nullptr
                ? static_cast<Timing::IClockSynchronizationTarget<Timing::ClockTick>*>(
                    &Timing::SystemClock<>::GetInstance()
                  )
                : target
        ) {
    }

    ~SNTPClockSyncProvider() {
        Shutdown();
    }

    bool Initialize(const SNTPClockSyncProviderConfig& config = {}) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_initialized) {
            return true;
        }

        if (
            _target == nullptr ||
            config.Server.length() == 0 ||
            config.UpdateIntervalMilliseconds == 0 ||
            (_activeInstance != nullptr && _activeInstance != this)
        ) {
            return false;
        }

        _config = config;
        _activeInstance = this;

        esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, const_cast<char*>(_config.Server.c_str()));
        esp_sntp_set_sync_interval(_config.UpdateIntervalMilliseconds);
        esp_sntp_set_time_sync_notification_cb(TimeSyncCallback);
        esp_sntp_init();

        _initialized = true;
        return true;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_initialized) {
            return;
        }

        if (_activeInstance == this) {
            esp_sntp_set_time_sync_notification_cb(nullptr);
            esp_sntp_stop();
            _activeInstance = nullptr;
        }

        _initialized = false;
    }

    bool GetIsInitialized() const noexcept {
        return _initialized;
    }

    Timing::ClockSynchronizationStatus<Timing::ClockTick>
    GetSynchronizationStatus() const {
        return _target->GetSynchronizationStatus();
    }
};

}
