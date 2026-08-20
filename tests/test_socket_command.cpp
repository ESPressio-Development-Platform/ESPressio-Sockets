#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <ESPressio_SocketCommandSession.hpp>

using namespace ESPressio;

static std::string AsString(const std::vector<uint8_t>& data) {
    return std::string(data.begin(), data.end());
}

static uint32_t FrameLength(const std::vector<uint8_t>& frame) {
    assert(frame.size() >= 4);
    return (static_cast<uint32_t>(frame[0]) << 24) |
           (static_cast<uint32_t>(frame[1]) << 16) |
           (static_cast<uint32_t>(frame[2]) << 8) |
           static_cast<uint32_t>(frame[3]);
}

static Sockets::SocketCommandResponse DecodeFramedResponse(const std::vector<uint8_t>& frame) {
    assert(frame.size() >= 4);
    const auto length = FrameLength(frame);
    assert(length == frame.size() - 4);
    Sockets::SocketCommandResponse response;
    assert(Sockets::SocketCommandProtocol::DecodeResponse(frame.data() + 4, length, response));
    return response;
}

int main() {
    {
        Command::CommandRegistry registry;
        auto& echo = registry.Command("echo");
        echo.Parameter<std::string>("value");
        echo.OnExecute([](const Command::CommandContext& context) {
            return Command::CommandResult::Ok(context.Get<std::string>("value"));
        });

        std::vector<uint8_t> output;
        Sockets::SocketCommandSession session;
        Sockets::SocketCommandSessionConfig config;
        config.Mode = Sockets::SocketCommandMode::Line;
        config.MaximumRequestBytes = 64;
        Sockets::SocketCommandMetadata metadata;
        metadata.Transport = "tcp";
        metadata.RemoteAddress = "192.0.2.10";
        metadata.RemotePort = 4444;
        metadata.SessionID = 7;
        assert(session.Initialize(registry, config, metadata,
            [&](const uint8_t* data, std::size_t size) {
                output.insert(output.end(), data, data + size);
                return true;
            }));

        const char* first = "echo hel";
        assert(session.Feed(reinterpret_cast<const uint8_t*>(first), std::strlen(first)));
        assert(output.empty());
        const char* second = "lo\r\n";
        assert(session.Feed(reinterpret_cast<const uint8_t*>(second), std::strlen(second)));
        assert(AsString(output) == "OK 0 hello\n");

        output.clear();
        const char* multiple = "echo one\necho two\n";
        assert(session.Feed(reinterpret_cast<const uint8_t*>(multiple), std::strlen(multiple)));
        assert(AsString(output) == "OK 0 one\nOK 0 two\n");

        output.clear();
        const char* quoted = "echo \"hello world\"\n";
        assert(session.Feed(reinterpret_cast<const uint8_t*>(quoted), std::strlen(quoted)));
        assert(AsString(output) == "OK 0 hello world\n");

        output.clear();
        const char* blank = "\n\r\n";
        assert(session.Feed(reinterpret_cast<const uint8_t*>(blank), std::strlen(blank)));
        assert(output.empty());

        bool policyCalled = false;
        bool observerCalled = false;
        session.SetPolicy([&](const Sockets::SocketCommandInvocationContext& context) {
            policyCalled = true;
            assert(context.Metadata.Transport == "tcp");
            assert(context.Metadata.RemoteAddress == "192.0.2.10");
            assert(context.Metadata.RemotePort == 4444);
            assert(context.Metadata.SessionID == 7);
            assert(context.Invocation.raw == "echo denied");
            return Command::CommandResult::Error("remote policy denied", 403);
        });
        session.SetResultObserver([&](const Sockets::SocketCommandInvocationContext& context, const Command::CommandResult& result) {
            observerCalled = true;
            assert(context.Metadata.RequestID > 0);
            assert(!result.success);
            assert(result.code == 403);
        });
        output.clear();
        const char* denied = "echo denied\n";
        assert(session.Feed(reinterpret_cast<const uint8_t*>(denied), std::strlen(denied)));
        assert(policyCalled);
        assert(observerCalled);
        assert(AsString(output) == "ERR 403 remote policy denied\n");

        session.SetPolicy({});
        session.SetResultObserver({});
        output.clear();
        const char* unknown = "does-not-exist\n";
        assert(session.Feed(reinterpret_cast<const uint8_t*>(unknown), std::strlen(unknown)));
        assert(AsString(output).find("ERR 1 Unknown command") == 0);
    }

    {
        Command::CommandRegistry registry;
        registry.Command("ping").OnExecute([](const Command::CommandContext&) {
            return Command::CommandResult::Ok("pong");
        });

        Sockets::SocketCommandSessionConfig config;
        config.MaximumRequestBytes = 8;
        std::vector<uint8_t> output;
        Sockets::SocketCommandSession session;
        assert(session.Initialize(registry, config, {}, [&](const uint8_t* data, std::size_t size) {
            output.insert(output.end(), data, data + size);
            return true;
        }));

        const char* oversized = "123456789012345\nping\n";
        assert(session.Feed(reinterpret_cast<const uint8_t*>(oversized), std::strlen(oversized)));
        assert(AsString(output) == "ERR 1 Command exceeds maximum request length\nOK 0 pong\n");
    }

    {
        Command::CommandRegistry registry;
        auto& add = registry.Command("math").Command("add");
        add.Parameter<int>("left");
        add.Parameter<int>("right");
        add.OnExecute([](const Command::CommandContext& context) {
            const int result = context.Get<int>("left") + context.Get<int>("right");
            return Command::CommandResult::Ok(std::to_string(result));
        });

        Sockets::SocketCommandInvocationContext request;
        request.Metadata.RequestID = 42;
        request.Invocation.path = {"math", "add"};
        request.Invocation.positional = {"20", "22"};
        request.Invocation.named["unused"] = "metadata-like-value";
        request.Invocation.raw = "machine request";

        std::vector<uint8_t> encoded;
        assert(Sockets::SocketCommandProtocol::EncodeRequest(request, encoded));
        Sockets::SocketCommandInvocationContext decoded;
        assert(Sockets::SocketCommandProtocol::DecodeRequest(encoded.data(), encoded.size(), decoded));
        assert(decoded.Metadata.RequestID == 42);
        assert(decoded.Invocation.path == request.Invocation.path);
        assert(decoded.Invocation.positional == request.Invocation.positional);
        assert(decoded.Invocation.named == request.Invocation.named);
        assert(decoded.Invocation.raw == request.Invocation.raw);

        // Remove the deliberately unknown named argument before execution.
        request.Invocation.named.clear();
        encoded.clear();
        assert(Sockets::SocketCommandProtocol::EncodeRequest(request, encoded));
        auto framed = Sockets::SocketCommandProtocol::FrameStructuredPayload(encoded);

        std::vector<uint8_t> output;
        Sockets::SocketCommandSession session;
        Sockets::SocketCommandSessionConfig config;
        config.Mode = Sockets::SocketCommandMode::StructuredBinary;
        config.MaximumRequestBytes = 512;
        Sockets::SocketCommandMetadata metadata;
        metadata.Transport = "tcp";
        metadata.SessionID = 99;
        assert(session.Initialize(registry, config, metadata, [&](const uint8_t* data, std::size_t size) {
            output.insert(output.end(), data, data + size);
            return true;
        }));

        bool policyCalled = false;
        session.SetPolicy([&](const Sockets::SocketCommandInvocationContext& context) {
            policyCalled = true;
            assert(context.Metadata.Transport == "tcp");
            assert(context.Metadata.SessionID == 99);
            assert(context.Metadata.RequestID >= 42 && context.Metadata.RequestID <= 44);
            assert(context.Invocation.path.size() == 2);
            return Command::CommandResult::Ok();
        });

        assert(session.Feed(framed.data(), 2));
        assert(output.empty());
        assert(session.Feed(framed.data() + 2, 3));
        assert(output.empty());
        assert(session.Feed(framed.data() + 5, framed.size() - 5));
        assert(policyCalled);

        auto response = DecodeFramedResponse(output);
        assert(response.RequestID == 42);
        assert(response.Result.success);
        assert(response.Result.code == 0);
        assert(response.Result.message == "42");

        // Multiple framed requests in one receive buffer.
        output.clear();
        request.Metadata.RequestID = 43;
        request.Invocation.positional = {"1", "2"};
        encoded.clear();
        assert(Sockets::SocketCommandProtocol::EncodeRequest(request, encoded));
        auto frame2 = Sockets::SocketCommandProtocol::FrameStructuredPayload(encoded);
        request.Metadata.RequestID = 44;
        request.Invocation.positional = {"3", "4"};
        encoded.clear();
        assert(Sockets::SocketCommandProtocol::EncodeRequest(request, encoded));
        auto frame3 = Sockets::SocketCommandProtocol::FrameStructuredPayload(encoded);
        std::vector<uint8_t> combined = frame2;
        combined.insert(combined.end(), frame3.begin(), frame3.end());
        assert(session.Feed(combined.data(), combined.size()));
        assert(!output.empty());

        const auto firstLength = static_cast<std::size_t>(FrameLength(output)) + 4;
        assert(firstLength < output.size());
        std::vector<uint8_t> firstResponse(output.begin(), output.begin() + firstLength);
        std::vector<uint8_t> secondResponse(output.begin() + firstLength, output.end());
        assert(DecodeFramedResponse(firstResponse).RequestID == 43);
        assert(DecodeFramedResponse(firstResponse).Result.message == "3");
        assert(DecodeFramedResponse(secondResponse).RequestID == 44);
        assert(DecodeFramedResponse(secondResponse).Result.message == "7");
    }

    {
        // Per-session framing state must be independent.
        Command::CommandRegistry registry;
        auto& echo = registry.Command("echo");
        echo.Parameter<std::string>("value");
        echo.OnExecute([](const Command::CommandContext& context) {
            return Command::CommandResult::Ok(context.Get<std::string>("value"));
        });
        Sockets::SocketCommandSessionConfig config;
        std::vector<uint8_t> outA, outB;
        Sockets::SocketCommandSession a, b;
        assert(a.Initialize(registry, config, {}, [&](const uint8_t* d, std::size_t n){ outA.insert(outA.end(), d, d+n); return true; }));
        assert(b.Initialize(registry, config, {}, [&](const uint8_t* d, std::size_t n){ outB.insert(outB.end(), d, d+n); return true; }));
        const char* pa = "echo A";
        const char* pb = "echo B\n";
        assert(a.Feed(reinterpret_cast<const uint8_t*>(pa), std::strlen(pa)));
        assert(b.Feed(reinterpret_cast<const uint8_t*>(pb), std::strlen(pb)));
        assert(outA.empty());
        assert(AsString(outB) == "OK 0 B\n");
        const char nl = '\n';
        assert(a.Feed(reinterpret_cast<const uint8_t*>(&nl), 1));
        assert(AsString(outA) == "OK 0 A\n");
    }

    {
        Command::CommandRegistry registry;
        registry.Command("ping").OnExecute([](const Command::CommandContext&) {
            return Command::CommandResult::Ok("pong");
        });
        Sockets::SocketCommandSessionConfig config;
        config.MaximumRequestBytes = 4;
        config.DisconnectOnProtocolError = true;
        std::vector<uint8_t> output;
        Sockets::SocketCommandSession session;
        assert(session.Initialize(registry, config, {}, [&](const uint8_t* d, std::size_t n) {
            output.insert(output.end(), d, d + n);
            return true;
        }));
        const char* oversized = "12345\n";
        assert(!session.Feed(reinterpret_cast<const uint8_t*>(oversized), std::strlen(oversized)));
        assert(AsString(output) == "ERR 1 Command exceeds maximum request length\n");
    }

    {
        // Protocol rejects truncation, invalid magic/version and malformed response state.
        Sockets::SocketCommandInvocationContext request;
        request.Metadata.RequestID = 1;
        request.Invocation.path = {"x"};
        std::vector<uint8_t> encoded;
        assert(Sockets::SocketCommandProtocol::EncodeRequest(request, encoded));
        Sockets::SocketCommandInvocationContext decoded;
        assert(!Sockets::SocketCommandProtocol::DecodeRequest(encoded.data(), encoded.size() - 1, decoded));
        auto badMagic = encoded;
        badMagic[0] ^= 0xFF;
        assert(!Sockets::SocketCommandProtocol::DecodeRequest(badMagic.data(), badMagic.size(), decoded));
        auto badVersion = encoded;
        badVersion[4] = 99;
        assert(!Sockets::SocketCommandProtocol::DecodeRequest(badVersion.data(), badVersion.size(), decoded));
    }

    return 0;
}
