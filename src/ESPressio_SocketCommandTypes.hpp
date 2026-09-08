#pragma once

#if !__has_include(<ESPressio_Command.hpp>)
#error "ESPressio Socket Command integration requires ESPressio Command >= 1.0.0 < 2.0.0."
#endif

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <ESPressio_Command.hpp>

namespace ESPressio::Sockets {

/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
enum
/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 1 bytes [0 bytes dynamic allocation]
 * Members: none (empty object still occupies at least 1 byte unless empty-base optimisation applies).
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
class SocketCommandMode : uint8_t {
    Line = 0,
    StructuredBinary = 1
};

/**
 * ESPressio Memory Audit
 * Members:
 * - Transport (std::string): 24 bytes [Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * - RemoteAddress (std::string): 24 bytes [Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * - RemotePort (uint16_t): 2 bytes [0 bytes dynamic allocation]
 * - SessionID (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * - RequestID (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * Total Memory: 68 bytes [Transport: Capacity + 1 bytes when capacity exceeds 15-byte SSO; RemoteAddress: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct SocketCommandMetadata {
    std::string Transport = "socket";
    std::string RemoteAddress;
    uint16_t RemotePort = 0;
    uint64_t SessionID = 0;
    uint64_t RequestID = 0;
};

/**
 * ESPressio Memory Audit
 * Members:
 * - Invocation (Command::CommandInvocation): sizeof(Command::CommandInvocation) [0 bytes dynamic allocation]
 * - Metadata (SocketCommandMetadata): 68 bytes [Transport: Capacity + 1 bytes when capacity exceeds 15-byte SSO; RemoteAddress: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Total Memory: 68 bytes known members + sizeof(Command::CommandInvocation) [Metadata: Transport: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Metadata: RemoteAddress: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct SocketCommandInvocationContext {
    Command::CommandInvocation Invocation;
    SocketCommandMetadata Metadata;
};

/**
 * ESPressio Memory Audit
 * Members:
 * - Mode (SocketCommandMode): 1 bytes [0 bytes dynamic allocation]
 * - MaximumRequestBytes (std::size_t): 4 bytes [0 bytes dynamic allocation]
 * - DisconnectOnProtocolError (bool): 1 bytes [0 bytes dynamic allocation]
 * - IgnoreEmptyLines (bool): 1 bytes [0 bytes dynamic allocation]
 * Total Memory: 12 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * End ESPressio Memory Audit
 */
struct SocketCommandSessionConfig {
    SocketCommandMode Mode = SocketCommandMode::Line;
    std::size_t MaximumRequestBytes = 1024;
    bool DisconnectOnProtocolError = false;
    bool IgnoreEmptyLines = true;
};

/**
 * ESPressio Memory Audit
 * Members:
 * - RequestID (uint64_t): 8 bytes [0 bytes dynamic allocation]
 * - Result (Command::CommandResult): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 12 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
struct SocketCommandResponse {
    uint64_t RequestID = 0;
    Command::CommandResult Result;
};

using SocketCommandWriteHandler =
    std::function<bool(const uint8_t*, std::size_t)>;

using SocketCommandPolicyHandler =
    std::function<Command::CommandResult(const SocketCommandInvocationContext&)>;

using SocketCommandResultObserver =
    std::function<void(const SocketCommandInvocationContext&, const Command::CommandResult&)>;

}
