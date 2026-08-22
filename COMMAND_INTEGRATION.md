# ESPressio Sockets Command Integration

ESPressio Sockets 0.7.1 provides opt-in integration with **ESPressio Command >= 1.0.1 < 2.0.0**.

Core ESPressio Sockets remains independent of ESPressio Command. Command support is activated only when the corresponding integration headers are selected.

## Dependency direction

```text
ESPressio Sockets core
    -> no Command dependency

Socket Command integration
    - - -> ESPressio Command >= 1.0.1 < 2.0.0
```

`SocketCommandSession` owns byte-stream framing, bounded request accumulation, socket-side metadata, policy hooks and result transport. ESPressio Command continues to own Command definition, parsing, typed parameter validation, routing and callback execution.

## Architecture

```text
TCP client
    |
    v
TCPCommandServer
    |
    +-- per-client SocketCommandSession
    +-- framing / bounded buffering
    +-- connection metadata
    +-- policy / result hooks
    |
    v
ESPressio CommandRegistry
    |
    v
typed Command callback
    |
    v
CommandResult
    |
    v
originating TCP client
```

## Public headers

```cpp
#include <ESPressio_SocketCommandTypes.hpp>
#include <ESPressio_SocketCommandProtocol.hpp>
#include <ESPressio_SocketCommandSession.hpp>
#include <ESPressio_TCPCommandServer.hpp>
```

These headers are deliberately not included by `ESPressio_Sockets.hpp`, preserving the optional dependency.

## TCP Command server

`TCPCommandServer` accepts multiple TCP clients and assigns an independent `SocketCommandSession` to each connection.

```cpp
#include <ESPressio_Command.hpp>
#include <ESPressio_TCPCommandServer.hpp>

using namespace ESPressio;

Sockets::TCPCommandServer server;

auto& commands = Command::CommandRegistry::GetInstance();
commands.Command("system")
    .Command("status")
    .OnExecute([](const Command::CommandContext&) {
        return Command::CommandResult::Ok("System OK");
    });

Sockets::TCPCommandServerConfig config;
config.Port = 2323;
config.MaximumClients = 4;
config.Session.Mode = Sockets::SocketCommandMode::Line;
config.Session.MaximumRequestBytes = 512;

server.Initialize(config, commands);
```

The consuming application remains responsible for establishing Wi-Fi/network connectivity before the server is initialized.

## Line-oriented mode

Line mode is intended for interactive or simple text clients.

Requests use normal ESPressio Command syntax:

```text
system status
gpio write 2 high
```

Each completed request produces one newline-delimited response:

```text
OK 0 System OK
ERR 1 Unknown command 'example'
```

LF and CRLF input are accepted. Quoting, escaping, Command-tree resolution and typed parameter parsing are delegated to ESPressio Command.

Fragmented TCP reads are accumulated until a complete line is available. Multiple complete commands received in one TCP read are processed independently.

## Structured-binary mode

Machine callers can avoid manufacturing command-line text by sending a structured `CommandInvocation` representation.

Command 1.0.0 allows positional and named invocation values to retain native scalar `CommandValue` types. Sockets accepts those typed invocations directly:

```cpp
Sockets::SocketCommandInvocationContext request;
request.Metadata.RequestID = 42;
request.Invocation.path = {"gpio", "write"};
request.Invocation.named["pin"] = 2;
request.Invocation.named["state"] = true;
```

### Protocol-v1 compatibility

The existing Sockets structured Command **wire protocol remains version 1**. It was originally defined using string-valued parameters, so Sockets 0.7.1 deliberately preserves that representation rather than introducing an incompatible wire revision.

At encode time:

```text
CommandValue -> CommandValue::ToString() -> protocol-v1 string
```

At decode time:

```text
protocol-v1 string -> string-backed CommandValue
```

The receiving Command Registry still performs the authoritative parameter conversion and validation, so values such as `"2"`, `"true"`, and `"0.5"` resolve through the same typed parameter definitions as before.

This preserves interoperability with existing protocol-v1 peers. Native scalar type identity is not carried over protocol v1. A null `CommandValue` has no representation in protocol v1 and is rejected rather than silently converted.

The version-1 request contains:

```text
magic
protocol version
request/correlation ID
Command path
positional parameters
named parameters
raw caller string
```

The version-1 response contains:

```text
magic
protocol version
matching request/correlation ID
success/failure
CommandResult code
CommandResult message
```

Structured payloads use a 32-bit length prefix and an ESPressio-owned binary encoding.

This mode intentionally does **not** require ESPressio Serializable or JSON.

The protocol helpers are exposed through `ESPressio_SocketCommandProtocol.hpp`:

```cpp
SocketCommandProtocol::EncodeRequest(...)
SocketCommandProtocol::DecodeRequest(...)
SocketCommandProtocol::EncodeResponse(...)
SocketCommandProtocol::DecodeResponse(...)
SocketCommandProtocol::FrameStructuredPayload(...)
```

## Session isolation

Each TCP client receives an independent `SocketCommandSession`.

The following state is therefore isolated per connection:

- partial line input;
- structured-frame accumulation;
- request/correlation state;
- remote address/port metadata;
- session ID;
- protocol recovery state.

A partial request from one client can never be completed by bytes arriving from another client.

## Resource limits

`SocketCommandSessionConfig` exposes:

```text
Mode
MaximumRequestBytes
DisconnectOnProtocolError
IgnoreEmptyLines
```

Line requests are bounded by `MaximumRequestBytes`. Oversized lines are discarded through the next newline before normal processing resumes.

Structured requests declare their payload length before execution and are rejected if the declared payload exceeds the configured limit.

`TCPCommandServerConfig::MaximumClients` is additionally bounded by `ESPRESSIO_SOCKETS_MAX_TCP_CLIENTS`.

## Connection metadata

Each invocation is associated with `SocketCommandMetadata`:

```text
Transport
RemoteAddress
RemotePort
SessionID
RequestID
```

`TCPCommandServer` populates the TCP connection fields automatically.

This metadata is supplied to the socket-side policy and result-observer hooks so applications can implement authorization, rate limiting, audit, diagnostics or transport-specific policy without coupling those concerns to Command callbacks.

## Policy hook

A server can apply a policy before a remote Command is executed:

```cpp
server.SetPolicy(
    [](const Sockets::SocketCommandInvocationContext& context) {
        if (context.Metadata.SessionID == 0) {
            return Command::CommandResult::Error(
                "Invalid remote session"
            );
        }

        return Command::CommandResult::Ok();
    }
);
```

Returning an error prevents the Command callback from executing and returns that `CommandResult` to the originating client.

## Result observation

Completed remote invocations can be observed independently of application Command callbacks:

```cpp
server.SetResultObserver(
    [](const Sockets::SocketCommandInvocationContext& context,
       const Command::CommandResult& result) {
        // Diagnostics or audit handling.
    }
);
```

## Command and Event semantics

Socket Command and Event integrations are complementary:

```text
Command
    remote caller requests an action

Event
    a device reports that something happened
```

A typical application may therefore receive a Command over TCP, perform the requested operation, then dispatch an Event describing the resulting state change.

## PlatformIO

```ini
lib_deps =
    espressio-development-platform/ESPressio-Sockets@^0.7.1
    espressio-development-platform/ESPressio-Command@^1.0.1
```

The existing Event and Timing dependencies remain required only when their corresponding Sockets integrations are selected.

## Example

See:

```text
examples/TCPCommandServer/TCPCommandServer.ino
```

for an ESP32 example registering application Commands and exposing them through the TCP Command server.
