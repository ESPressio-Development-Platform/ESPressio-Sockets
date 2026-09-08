#pragma once

#include <string>

#include <ESPressio_Event.hpp>

namespace ESPressio::Event {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Name (std::string): 24 bytes [Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Total Memory: 48 bytes [Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SocketWorkerStartedEvent final : public TypedEvent<SocketWorkerStartedEvent> {
public:
    const std::string Name;
    explicit SocketWorkerStartedEvent(const char* name) : Name(name == nullptr ? "" : name) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Name (std::string): 24 bytes [Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Total Memory: 48 bytes [Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SocketWorkerStartFailedEvent final : public TypedEvent<SocketWorkerStartFailedEvent> {
public:
    const std::string Name;
    explicit SocketWorkerStartFailedEvent(const char* name) : Name(name == nullptr ? "" : name) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members: none (standalone empty object occupies 1 byte; an eligible empty base may be optimized to 0 bytes).
 * Total Memory: 24 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SocketWorkerStoppedEvent final : public TypedEvent<SocketWorkerStoppedEvent> {};

} // namespace ESPressio::Event
