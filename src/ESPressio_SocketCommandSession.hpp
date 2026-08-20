#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "ESPressio_SocketCommandProtocol.hpp"
#include "ESPressio_SocketCommandTypes.hpp"

namespace ESPressio::Sockets {

class SocketCommandSession final {
public:
    SocketCommandSession() = default;

    bool Initialize(
        Command::CommandRegistry& registry,
        const SocketCommandSessionConfig& config,
        SocketCommandMetadata metadata,
        SocketCommandWriteHandler writer
    ) {
        Shutdown();
        if (!writer || config.MaximumRequestBytes == 0) return false;
        _registry = &registry;
        _config = config;
        _metadata = std::move(metadata);
        _writer = std::move(writer);
        _initialized = true;
        return true;
    }

    void Shutdown() {
        _registry = nullptr;
        _writer = {};
        _policy = {};
        _observer = {};
        _line.clear();
        _structured.clear();
        _discardUntilNewline = false;
        _expectedStructuredBytes = 0;
        _initialized = false;
    }

    void ResetInput() {
        _line.clear();
        _structured.clear();
        _discardUntilNewline = false;
        _expectedStructuredBytes = 0;
    }

    bool GetIsInitialized() const noexcept { return _initialized; }
    const SocketCommandMetadata& GetMetadata() const noexcept { return _metadata; }
    void SetMetadata(SocketCommandMetadata metadata) { _metadata = std::move(metadata); }
    void SetPolicy(SocketCommandPolicyHandler policy) { _policy = std::move(policy); }
    void SetResultObserver(SocketCommandResultObserver observer) { _observer = std::move(observer); }

    bool Feed(const uint8_t* data, std::size_t size) {
        if (!_initialized || data == nullptr) return false;
        return _config.Mode == SocketCommandMode::Line
            ? FeedLine(data, size)
            : FeedStructured(data, size);
    }

private:
    Command::CommandRegistry* _registry = nullptr;
    SocketCommandSessionConfig _config;
    SocketCommandMetadata _metadata;
    SocketCommandWriteHandler _writer;
    SocketCommandPolicyHandler _policy;
    SocketCommandResultObserver _observer;
    std::string _line;
    std::vector<uint8_t> _structured;
    bool _discardUntilNewline = false;
    uint32_t _expectedStructuredBytes = 0;
    bool _initialized = false;

    static std::string Trim(std::string value) {
        auto notSpace = [](unsigned char c) { return !std::isspace(c); };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
        value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
        return value;
    }

    Command::CommandResult Execute(SocketCommandInvocationContext& context) {
        if (_registry == nullptr) return Command::CommandResult::Error("Socket Command session is not initialized");
        if (_policy) {
            auto policyResult = _policy(context);
            if (!policyResult.success) {
                if (_observer) _observer(context, policyResult);
                return policyResult;
            }
        }
        auto result = _registry->Invoke(context.Invocation);
        if (_observer) _observer(context, result);
        return result;
    }

    bool WriteLineResult(const Command::CommandResult& result) {
        std::string response = result.success ? "OK " : "ERR ";
        response += std::to_string(result.code);
        if (!result.message.empty()) {
            response.push_back(' ');
            response += result.message;
        }
        response.push_back('\n');
        return _writer(reinterpret_cast<const uint8_t*>(response.data()), response.size());
    }

    bool HandleLine(std::string line) {
        line = Trim(std::move(line));
        if (line.empty() && _config.IgnoreEmptyLines) return true;

        SocketCommandInvocationContext context;
        context.Metadata = _metadata;
        ++context.Metadata.RequestID;
        _metadata.RequestID = context.Metadata.RequestID;
        context.Invocation.raw = line;

        std::string parseError;
        auto tokens = Command::TextCommandParser::Tokenize(line, &parseError);
        if (!parseError.empty()) return WriteLineResult(Command::CommandResult::Error(parseError));
        if (tokens.empty()) return WriteLineResult(Command::CommandResult::Error("No command supplied"));

        // Text Commands are still parsed/executed by ESPressio Command, while
        // Sockets exposes transport metadata to the network policy layer.
        context.Invocation.path = {tokens.front()};
        if (_policy) {
            auto policyResult = _policy(context);
            if (!policyResult.success) {
                if (_observer) _observer(context, policyResult);
                return WriteLineResult(policyResult);
            }
        }
        auto result = _registry->Invoke(line);
        if (_observer) _observer(context, result);
        return WriteLineResult(result);
    }

    bool FeedLine(const uint8_t* data, std::size_t size) {
        bool success = true;
        for (std::size_t i = 0; i < size; ++i) {
            const char c = static_cast<char>(data[i]);
            if (c == '\r') continue;
            if (c == '\n') {
                if (_discardUntilNewline) {
                    _discardUntilNewline = false;
                    _line.clear();
                    const bool wrote = WriteLineResult(Command::CommandResult::Error("Command exceeds maximum request length"));
                    if (_config.DisconnectOnProtocolError) return false;
                    success = wrote && success;
                    continue;
                }
                std::string line = std::move(_line);
                _line.clear();
                success = HandleLine(std::move(line)) && success;
                continue;
            }
            if (_discardUntilNewline) continue;
            if (_line.size() >= _config.MaximumRequestBytes) {
                _line.clear();
                _discardUntilNewline = true;
                continue;
            }
            _line.push_back(c);
        }
        return success;
    }

    static uint32_t ReadFrameLength(const uint8_t* data) {
        return (static_cast<uint32_t>(data[0]) << 24) |
               (static_cast<uint32_t>(data[1]) << 16) |
               (static_cast<uint32_t>(data[2]) << 8) |
               static_cast<uint32_t>(data[3]);
    }

    bool EmitStructuredError(uint64_t requestID, std::string message) {
        SocketCommandResponse response;
        response.RequestID = requestID;
        response.Result = Command::CommandResult::Error(std::move(message));
        std::vector<uint8_t> payload;
        if (!SocketCommandProtocol::EncodeResponse(response, payload)) return false;
        auto frame = SocketCommandProtocol::FrameStructuredPayload(payload);
        return !frame.empty() && _writer(frame.data(), frame.size());
    }

    bool HandleStructuredFrame(const uint8_t* data, std::size_t size) {
        SocketCommandInvocationContext context;
        context.Metadata = _metadata;
        if (!SocketCommandProtocol::DecodeRequest(data, size, context)) {
            return EmitStructuredError(0, "Malformed structured Command request");
        }
        _metadata.RequestID = context.Metadata.RequestID;
        auto result = Execute(context);
        SocketCommandResponse response;
        response.RequestID = context.Metadata.RequestID;
        response.Result = std::move(result);
        std::vector<uint8_t> payload;
        if (!SocketCommandProtocol::EncodeResponse(response, payload)) return false;
        auto frame = SocketCommandProtocol::FrameStructuredPayload(payload);
        return !frame.empty() && _writer(frame.data(), frame.size());
    }

    bool FeedStructured(const uint8_t* data, std::size_t size) {
        if (size > _config.MaximumRequestBytes + 4 && _structured.empty()) return false;
        _structured.insert(_structured.end(), data, data + size);
        bool success = true;

        while (true) {
            if (_expectedStructuredBytes == 0) {
                if (_structured.size() < 4) break;
                _expectedStructuredBytes = ReadFrameLength(_structured.data());
                _structured.erase(_structured.begin(), _structured.begin() + 4);
                if (_expectedStructuredBytes == 0 || _expectedStructuredBytes > _config.MaximumRequestBytes) {
                    _structured.clear();
                    _expectedStructuredBytes = 0;
                    const bool wrote = EmitStructuredError(0, "Structured Command frame exceeds configured limit");
                    return _config.DisconnectOnProtocolError ? false : wrote;
                }
            }
            if (_structured.size() < _expectedStructuredBytes) break;

            const auto frameSize = static_cast<std::size_t>(_expectedStructuredBytes);
            success = HandleStructuredFrame(_structured.data(), frameSize) && success;
            _structured.erase(_structured.begin(), _structured.begin() + frameSize);
            _expectedStructuredBytes = 0;
        }

        if (_structured.size() > _config.MaximumRequestBytes) {
            _structured.clear();
            _expectedStructuredBytes = 0;
            return false;
        }
        return success;
    }
};

}
