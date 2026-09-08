#pragma once

#include <ESPressio_Event.hpp>
#include <ESPressio_SecurityTypes.hpp>

namespace ESPressio::Event {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + sizeof(std::atomic_flag) [0 bytes dynamic allocation]
 * Members:
 * - Result (Security::SecurityResult): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 4 bytes known bases + sizeof(std::atomic_flag) + 4 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SocketSecuritySessionFaultedEvent final : public TypedEvent<SocketSecuritySessionFaultedEvent> {
public:
    const Security::SecurityResult Result;
    explicit SocketSecuritySessionFaultedEvent(const Security::SecurityResult& result) : Result(result) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + sizeof(std::atomic_flag) [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 4 bytes known bases + sizeof(std::atomic_flag) [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SocketSecuritySessionResetEvent final : public TypedEvent<SocketSecuritySessionResetEvent> {};

} // namespace ESPressio::Event
