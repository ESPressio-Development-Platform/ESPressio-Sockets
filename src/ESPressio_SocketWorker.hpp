#pragma once

#include <atomic>

#include "ESPressio_SocketTypes.hpp"

namespace ESPressio::Sockets {

class SocketWorker {
private:
    TaskHandle_t _taskHandle = nullptr;
    std::atomic<bool> _running{false};
    SocketWorkerConfig _config;

    static void TaskEntry(
        void* parameter
    ) {
        auto* worker =
            static_cast<SocketWorker*>(
                parameter
            );

        if (worker != nullptr) {
            worker->Run();
        }

        vTaskDelete(nullptr);
    }

    void Run() {
        while (_running.load()) {
            OnWorkerIteration();

            if (
                _config.IdleDelayMilliseconds >
                0
            ) {
                vTaskDelay(
                    pdMS_TO_TICKS(
                        _config.
                            IdleDelayMilliseconds
                    )
                );
            } else {
                taskYIELD();
            }
        }

        _taskHandle = nullptr;
    }

protected:
    virtual void OnWorkerIteration() = 0;

    bool StartWorker(
        const char* name,
        const SocketWorkerConfig& config
    ) {
        if (_running.load()) {
            return true;
        }

        _config = config;
        _running.store(true);

        const BaseType_t result =
            xTaskCreatePinnedToCore(
                TaskEntry,
                name,
                config.StackSize,
                this,
                config.Priority,
                &_taskHandle,
                config.Core
            );

        if (result != pdPASS) {
            _running.store(false);
            _taskHandle = nullptr;
            return false;
        }

        return true;
    }

    void StopWorker() {
        _running.store(false);

        if (
            _taskHandle == nullptr ||
            xTaskGetCurrentTaskHandle() ==
                _taskHandle
        ) {
            return;
        }

        /*
         * Worker loops are deliberately non-blocking or use short timeouts.
         * Wait for natural exit before derived classes destroy their socket
         * resources.
         */
        while (_taskHandle != nullptr) {
            vTaskDelay(
                pdMS_TO_TICKS(1)
            );
        }
    }

public:
    virtual ~SocketWorker() {
        StopWorker();
    }

    bool GetWorkerIsRunning() const
        noexcept {
        return _running.load();
    }
};

}
