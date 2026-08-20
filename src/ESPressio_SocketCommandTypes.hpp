#pragma once

#if !__has_include(<ESPressio_Command.hpp>)
#error "ESPressio Socket Command integration requires ESPressio Command >= 0.2.0 < 1.0.0."
#endif

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <ESPressio_Command.hpp>

namespace ESPressio::Sockets {

enum class SocketCommandMode : uint8_t {
    Line = 0,
    StructuredBinary = 1
};

struct SocketCommandMetadata {
    std::string Transport = "socket";
    std::string RemoteAddress;
    uint16_t RemotePort = 0;
    uint64_t SessionID = 0;
    uint64_t RequestID = 0;
};

struct SocketCommandInvocationContext {
    Command::CommandInvocation Invocation;
    SocketCommandMetadata Metadata;
};

struct SocketCommandSessionConfig {
    SocketCommandMode Mode = SocketCommandMode::Line;
    std::size_t MaximumRequestBytes = 1024;
    bool DisconnectOnProtocolError = false;
    bool IgnoreEmptyLines = true;
};

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
