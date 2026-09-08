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
 * - _executionHandle (std::atomic<System::Execution::ExecutionHandle>): 4 bytes [0 bytes dynamic allocation]
 * - _running (std::atomic<bool>): 1 bytes [0 bytes dynamic allocation]
 * - _config (SocketWorkerConfig): 16 bytes [0 bytes dynamic allocation]
 * - _observable (std::shared_ptr<WorkerObservable>): 8 bytes [shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Total Memory: 36 bytes [_observable: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 96 bytes; _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; _observable: pointee: ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; _observable: pointee: ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; _observable: pointee: ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; _observable: pointee: ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; _observable: pointee: ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; _observable: pointee: ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; _observable: pointee: ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SocketWorker {
private:
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 96 bytes [ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Members: none (standalone empty object occupies 1 byte; an eligible empty base may be optimized to 0 bytes).
 * Total Memory: 96 bytes [ThreadSafeObservable: Observable: IUntypedObservable: IObservable: enable_shared_from_this: embedded weak_ptr shares a control block when activated; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: shared control block (~12+ bytes; allocate_shared may co-locate object) + object 20 bytes; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: Observable: IUntypedObservable: IObservable: _lifetimeControl: pointee: _condition: native condition-variable state may allocate platform synchronization resources; ThreadSafeObservable: Observable: _registrations: Capacity * (12 bytes) element storage; ThreadSafeObservable: Observable: _bindings: Capacity * (12 bytes) element storage; ThreadSafeObservable: _mutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _mutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily; ThreadSafeObservable: _notificationMutex: _owned: owned object: 4 bytes; ThreadSafeObservable: _notificationMutex: _fallback: _mutex: native synchronization state may allocate platform resources lazily]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
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
