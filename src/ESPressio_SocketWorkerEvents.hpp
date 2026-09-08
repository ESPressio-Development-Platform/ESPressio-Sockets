#pragma once

#include <string>

#include <ESPressio_Event.hpp>

namespace ESPressio::Event {

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + sizeof(std::atomic_flag) [0 bytes dynamic allocation]
 * Members:
 * - Name (std::string): 24 bytes [Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Total Memory: 4 bytes known bases + sizeof(std::atomic_flag) + 24 bytes known members [Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SocketWorkerStartedEvent final : public TypedEvent<SocketWorkerStartedEvent> {
public:
    const std::string Name;
    explicit SocketWorkerStartedEvent(const char* name) : Name(name == nullptr ? "" : name) {}
};

/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + sizeof(std::atomic_flag) [0 bytes dynamic allocation]
 * Members:
 * - Name (std::string): 24 bytes [Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Total Memory: 4 bytes known bases + sizeof(std::atomic_flag) + 24 bytes known members [Name: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
class SocketWorkerStartFailedEvent final : public TypedEvent<SocketWorkerStartFailedEvent> {
public:
    const std::string Name;
    explicit SocketWorkerStartFailedEvent(const char* name) : Name(name == nullptr ? "" : name) {}
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
class SocketWorkerStoppedEvent final : public TypedEvent<SocketWorkerStoppedEvent> {};

} // namespace ESPressio::Event
