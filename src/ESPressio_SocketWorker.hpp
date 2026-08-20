#pragma once

#include <atomic>
#include <memory>

#include <ESPressio_ThreadSafeObservable.hpp>

#include "ESPressio_ISocketWorkerObserver.hpp"
#include "ESPressio_SocketTypes.hpp"

namespace ESPressio::Sockets {

class SocketWorker {
private:
    class WorkerObservable final : public Observable::ThreadSafeObservable {
    private:
        template <typename Callback>
        void Notify(Callback&& callback) {
            ExecuteNotification([&](NotificationContext& notification) {
                notification.WithObservers<ISocketWorkerObserver>([&](ISocketWorkerObserver* observer) {
                    try { callback(observer); } catch (...) {}
                });
            });
        }
    public:
        void Started(const char* name) { Notify([&](ISocketWorkerObserver* observer){ observer->OnSocketWorkerStarted(name); }); }
        void StartFailed(const char* name) { Notify([&](ISocketWorkerObserver* observer){ observer->OnSocketWorkerStartFailed(name); }); }
        void Stopped() { Notify([](ISocketWorkerObserver* observer){ observer->OnSocketWorkerStopped(); }); }
    };

    TaskHandle_t _taskHandle = nullptr;
    std::atomic<bool> _running{false};
    SocketWorkerConfig _config;
    std::shared_ptr<WorkerObservable> _observable = std::make_shared<WorkerObservable>();

    static void TaskEntry(void* parameter) {
        auto* worker = static_cast<SocketWorker*>(parameter);
        if (worker != nullptr) worker->Run();
        vTaskDelete(nullptr);
    }

    void Run() {
        while (_running.load()) {
            OnWorkerIteration();
            if (_config.IdleDelayMilliseconds > 0) {
                vTaskDelay(pdMS_TO_TICKS(_config.IdleDelayMilliseconds));
            } else {
                taskYIELD();
            }
        }
        _taskHandle = nullptr;
    }

protected:
    virtual void OnWorkerIteration() = 0;

    bool StartWorker(const char* name, const SocketWorkerConfig& config) {
        if (_running.load()) return true;

        _config = config;
        _running.store(true);

        const BaseType_t result = xTaskCreatePinnedToCore(
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
            _observable->StartFailed(name);
            return false;
        }

        _observable->Started(name);
        return true;
    }

    void StopWorker() {
        const bool wasRunning = _running.exchange(false);

        if (_taskHandle != nullptr && xTaskGetCurrentTaskHandle() != _taskHandle) {
            while (_taskHandle != nullptr) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }

        if (wasRunning) _observable->Stopped();
    }

public:
    virtual ~SocketWorker() { StopWorker(); }

    Observable::ObserverHandlePtr RegisterObserver(ISocketWorkerObserver* observer) {
        return _observable->RegisterObserver(observer);
    }

    void UnregisterObserver(ISocketWorkerObserver* observer) {
        _observable->UnregisterObserver(observer);
    }

    bool GetWorkerIsRunning() const noexcept { return _running.load(); }
};

} // namespace ESPressio::Sockets
