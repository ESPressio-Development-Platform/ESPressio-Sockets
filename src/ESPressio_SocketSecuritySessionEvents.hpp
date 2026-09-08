#pragma once

#include <ESPressio_Event.hpp>
#include <ESPressio_SecurityTypes.hpp>

namespace ESPressio::Event {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members:
 * - Result (Security::SecurityResult): 28 bytes [Message: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Total Memory: 52 bytes [Result: Message: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SocketSecuritySessionFaultedEvent final : public TypedEvent<SocketSecuritySessionFaultedEvent> {
public:
    const Security::SecurityResult Result;
    explicit SocketSecuritySessionFaultedEvent(const Security::SecurityResult& result) : Result(result) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 24 bytes [0 bytes dynamic allocation]
 * Members: none (standalone empty object occupies 1 byte; an eligible empty base may be optimized to 0 bytes).
 * Total Memory: 24 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class SocketSecuritySessionResetEvent final : public TypedEvent<SocketSecuritySessionResetEvent> {};

} // namespace ESPressio::Event
