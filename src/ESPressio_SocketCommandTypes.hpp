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
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
enum
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
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
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
 * - Invocation (Command::CommandInvocation): 76 bytes [path: Capacity * (24 bytes) element storage; path: N live elements each: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO; positional: Capacity * (28 bytes) element storage; positional: N live elements each: value_: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO; named: N * (16 bytes red-black-tree node linkage + 52 bytes value); named: key/value: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO; named: key/value: value_: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO; raw: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * - Metadata (SocketCommandMetadata): 68 bytes [Transport: Capacity + 1 bytes when capacity exceeds 15-byte SSO; RemoteAddress: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Total Memory: 144 bytes [Invocation: path: Capacity * (24 bytes) element storage; Invocation: path: N live elements each: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Invocation: positional: Capacity * (28 bytes) element storage; Invocation: positional: N live elements each: value_: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Invocation: named: N * (16 bytes red-black-tree node linkage + 52 bytes value); Invocation: named: key/value: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Invocation: named: key/value: value_: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Invocation: raw: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Metadata: Transport: Capacity + 1 bytes when capacity exceeds 15-byte SSO; Metadata: RemoteAddress: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
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
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
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
 * - Result (Command::CommandResult): 32 bytes [message: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Total Memory: 40 bytes [Result: message: CommandStringStorage: _value: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
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
