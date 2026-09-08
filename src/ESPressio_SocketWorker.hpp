#pragma once

#include <atomic>
#include <memory>

#include <ESPressio_Execution.hpp>
#include <ESPressio_ThreadSafeObservable.hpp>

#include "ESPressio_ISocketWorkerObserver.hpp"
#include "ESPressio_SocketTypes.hpp"

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Members:
 * - _config (SocketWorkerConfig): 12 bytes [0 bytes dynamic allocation]
 * Total Memory: 16 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class SocketWorker {
private:
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + sizeof(std::enable_shared_from_this<ThreadSafeObservable>) + 4 bytes known members + sizeof(std::recursive_mutex) [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 4 bytes known bases + sizeof(std::enable_shared_from_this<ThreadSafeObservable>) + 4 bytes known members + sizeof(std::recursive_mutex) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
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

    std::atomic<System::Execution::ExecutionHandle> _executionHandle{
        System::Execution::InvalidExecutionHandle
    };
    std::atomic<bool> _running{false};
    SocketWorkerConfig _config;
    std::shared_ptr<WorkerObservable> _observable = std::make_shared<WorkerObservable>();

    static void TaskEntry(void* parameter) {
        auto* worker = static_cast<SocketWorker*>(parameter);
        if (worker != nullptr) worker->Run();
        System::Execution::Provider().Destroy(
            System::Execution::Provider().Current()
        );
    }

    void Run() {
        while (_running.load(std::memory_order_acquire)) {
            OnWorkerIteration();
            if (_config.IdleDelayMilliseconds > 0) {
                System::Execution::Provider().SleepMilliseconds(
                    _config.IdleDelayMilliseconds
                );
            } else {
                System::Execution::Provider().Yield();
            }
        }
        _executionHandle.store(
            System::Execution::InvalidExecutionHandle,
            std::memory_order_release
        );
    }

protected:
    virtual void OnWorkerIteration() = 0;

    bool StartWorker(const char* name, const SocketWorkerConfig& config) {
        if (_running.load(std::memory_order_acquire)) return true;

        _config = config;
        _running.store(true, std::memory_order_release);

        System::Execution::ExecutionConfiguration execution;
        execution.Name = name;
        execution.StackSizeBytes = config.StackSize;
        execution.Priority = config.Priority;
        execution.Affinity = config.Affinity;

        const auto result = System::Execution::Provider().Create(
            &TaskEntry,
            this,
            execution
        );

        if (!result) {
            _running.store(false, std::memory_order_release);
            _executionHandle.store(
                System::Execution::InvalidExecutionHandle,
                std::memory_order_release
            );
            _observable->StartFailed(name);
            return false;
        }

        _executionHandle.store(result.Handle, std::memory_order_release);
        _observable->Started(name);
        return true;
    }

    void StopWorker() {
        const bool wasRunning = _running.exchange(false, std::memory_order_acq_rel);
        const auto handle = _executionHandle.load(std::memory_order_acquire);
        const auto current = System::Execution::Provider().Current();

        if (
            handle != System::Execution::InvalidExecutionHandle &&
            current != handle
        ) {
            while (
                _executionHandle.load(std::memory_order_acquire) !=
                    System::Execution::InvalidExecutionHandle
            ) {
                System::Execution::Provider().SleepMilliseconds(1);
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

    bool GetWorkerIsRunning() const noexcept {
        return _running.load(std::memory_order_acquire);
    }
};

} // namespace ESPressio::Sockets
